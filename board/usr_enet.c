#include "usr_enet.h"
#include "fsl_clock.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
#include "fsl_enet.h"
#include "fsl_phy.h"
#include "lwip/ip.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "lwip/timeouts.h"

#include "ethernetif.h"
#include "board.h"
#include "queue.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* DHCP Task */
#define VTASKDHCP_STK_ZISE 512
#define VTASKDHCP_TASK_PRIO 5
TaskHandle_t vTaskDHCP_Handle;
static void vTaskDHCP(void *arg);




/* IP address configuration. */
#define configIP_ADDR0 192
#define configIP_ADDR1 168
#define configIP_ADDR2 0
#define configIP_ADDR3 212

/* Netmask configuration. */
#define configNET_MASK0 255
#define configNET_MASK1 255
#define configNET_MASK2 255
#define configNET_MASK3 0

/* Gateway address configuration. */
#define configGW_ADDR0 192
#define configGW_ADDR1 168
#define configGW_ADDR2 0
#define configGW_ADDR3 0

/* MAC address configuration. */
#define configMAC_ADDR                     \
    {                                      \
        0x02, 0x12, 0x13, 0x10, 0x15, 0x11 \
    }

/* Address of PHY interface. */
#define EXAMPLE_PHY_ADDRESS BOARD_ENET0_PHY_ADDRESS

/* System clock name. */
#define EXAMPLE_CLOCK_NAME kCLOCK_CoreSysClk

/*******************************************************************************
* Variables
******************************************************************************/
static struct netif fsl_netif0;
extern QueueHandle_t IP_Queue; 


/*******************************************************************************
 * Code
 ******************************************************************************/
static void delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 1000000; ++i)
    {
        __asm("NOP"); /* delay */
    }
}
static void svusr_enetPrintDhcpState(struct netif *netif)
{
    static u8_t dhcp_last_state = DHCP_STATE_OFF;
    struct dhcp *dhcp = netif_dhcp_data(netif);

    if (dhcp == NULL)
    {
        dhcp_last_state = DHCP_STATE_OFF;
    }
    else if (dhcp_last_state != dhcp->state)
    {
        dhcp_last_state = dhcp->state;

        PRINTF(" DHCP state       : ");
        switch (dhcp_last_state)
        {
            case DHCP_STATE_OFF:
                PRINTF("OFF");
                break;
            case DHCP_STATE_REQUESTING:
                PRINTF("REQUESTING");
                break;
            case DHCP_STATE_INIT:
                PRINTF("INIT");
                break;
            case DHCP_STATE_REBOOTING:
                PRINTF("REBOOTING");
                break;
            case DHCP_STATE_REBINDING:
                PRINTF("REBINDING");
                break;
            case DHCP_STATE_RENEWING:
                PRINTF("RENEWING");
                break;
            case DHCP_STATE_SELECTING:
                PRINTF("SELECTING");
                break;
            case DHCP_STATE_INFORMING:
                PRINTF("INFORMING");
                break;
            case DHCP_STATE_CHECKING:
                PRINTF("CHECKING");
                break;
            case DHCP_STATE_BOUND:
                PRINTF("BOUND");
                break;
            case DHCP_STATE_BACKING_OFF:
                PRINTF("BACKING_OFF");
                break;
            default:
                PRINTF("%u", dhcp_last_state);
                assert(0);
                break;
        }
        PRINTF("\r\n");

        if (dhcp_last_state == DHCP_STATE_BOUND)
        {
            PRINTF("\r\n IPv4 Address     : %s\r\n", ipaddr_ntoa(&netif->ip_addr));
            PRINTF(" IPv4 Subnet mask : %s\r\n", ipaddr_ntoa(&netif->netmask));
            PRINTF(" IPv4 Gateway     : %s\r\n\r\n", ipaddr_ntoa(&netif->gw));
        }
    }
}
void vusr_enetInitModuleClock(void)
{
    const clock_enet_pll_config_t config = {true, false, 1};
    CLOCK_InitEnetPll(&config);
}

void vusr_enetInitPort(void)
{
    gpio_pin_config_t gpio_config = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    vusr_enetInitModuleClock();
    IOMUXC_EnableMode(IOMUXC_GPR, kIOMUXC_GPR_ENET1TxClkOutputDir, true);
    GPIO_PinInit(GPIO1, 9, &gpio_config);  // ENET_RST
    GPIO_PinInit(GPIO1, 10, &gpio_config); // ENET_INT
    /* pull up the ENET_INT before RESET. */
    GPIO_WritePinOutput(GPIO1, 10, 1);
    GPIO_WritePinOutput(GPIO1, 9, 0);
    delay();
    GPIO_WritePinOutput(GPIO1, 9, 1);
}

void vusr_enetInit(void)
{
    ip4_addr_t fsl_netif0_ipaddr, fsl_netif0_netmask, fsl_netif0_gw;
    ethernetif_config_t fsl_enet_config0 = {
        .phyAddress = EXAMPLE_PHY_ADDRESS,
        .clockName = EXAMPLE_CLOCK_NAME,
        .macAddress = configMAC_ADDR,
    };

    tcpip_init(NULL, NULL);

    IP4_ADDR(&fsl_netif0_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&fsl_netif0_netmask, 0, 0, 0, 0);
    IP4_ADDR(&fsl_netif0_gw, 0, 0, 0, 0);

    netif_add(&fsl_netif0, &fsl_netif0_ipaddr, &fsl_netif0_netmask, &fsl_netif0_gw,
              &fsl_enet_config0, ethernetif0_init, tcpip_input);
    netif_set_default(&fsl_netif0);
    netif_set_up(&fsl_netif0);
    dhcp_start(&fsl_netif0);
    // PRINTF("\r\n************************************************\r\n");
    // PRINTF(" DHCP example\r\n");
    // PRINTF("************************************************\r\n");
    if (xTaskCreate(vTaskDHCP, "vTaskDHCP", VTASKDHCP_STK_ZISE, NULL, VTASKDHCP_TASK_PRIO, vTaskDHCP_Handle) != pdPASS)
    {
        PRINTF("DHCP task create faild.\r\n");
        while (1)
            ;
    }
}

void vTaskDHCP(void *arg)
{
    while (netif_dhcp_data(&fsl_netif0)->state != DHCP_STATE_BOUND)
    {
        /* Poll the driver, get any outstanding frames */
        ethernetif_input(&fsl_netif0);
        /* Handle all system timeouts for all core protocols */
        vTaskDelay(200);
        /* Print DHCP progress */
        svusr_enetPrintDhcpState(&fsl_netif0);
    }
    xQueueSend(IP_Queue, &(fsl_netif0.ip_addr), 10);
	vTaskDelete(NULL);
}
