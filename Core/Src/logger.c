#include <string.h>
#include "rte.h"
#include "stm32f4xx_hal.h"
#include "usbd_cdc_if.h"
#include "util.h"

#define USE_RAW 1

enum Logger_Sts_type
{
    Logger_Sts_type_Header = 0,
    Logger_Sts_type_Data,
    Logger_Sts_type_NUM,
};

static char log_data[APP_TX_DATA_SIZE] = {0};
static enum Logger_Sts_type logger_sts = Logger_Sts_type_Header;

volatile uint8_t host_open = 0;
volatile uint8_t header_sent = 0;
volatile uint32_t host_open_tick = 0;
volatile uint8_t host_open_pending = 0;

static void logger_send_header(void);
static void logger_send_data(void);

static void (*logger_send_hdl[Logger_Sts_type_NUM])(void) = {
    logger_send_header,
    logger_send_data,
};

static void logger_send_header(void)
{
    if (host_open_pending && (HAL_GetTick() - host_open_tick > 50)) {
        host_open_pending = 0;
        host_open = 1;
    }

    if (!host_open) return;    // nobody listening — don't transmit

    int off = 0;
    off += sprintf(log_data, "timestamp_ms,current_A,stack_V,");
    for (uint8_t cell_id = 0; cell_id < AFEDRV_CELL_CNT; cell_id++) {
        off += sprintf(log_data + off, "cell%02u_V", cell_id);
        if (cell_id != (AFEDRV_CELL_CNT - 1)) {
            off += sprintf(log_data + off, ",");
        }
    }
    off += sprintf(log_data + off, "\r\n");
    if (HAL_OK == CDC_Transmit_FS((uint8_t*)log_data, off)) {
        logger_sts = Logger_Sts_type_Data;
    }
}

#if USE_RAW
static void logger_send_data(void)
{
    uint32_t timestamp_ms = HAL_GetTick();

    int off = 0;
    off += sprintf(log_data + off, "0x%08lX,", timestamp_ms);
    off += sprintf(log_data + off, "0x%08lX,", *(uint32_t*)&rte_monitor_data.current_A);
    off += sprintf(log_data + off, "0x%08lX,", *(uint32_t*)&rte_monitor_data.stack_V);

    for (uint8_t cell_id = 0; cell_id < AFEDRV_CELL_CNT; cell_id++) {
        off += sprintf(log_data + off, "0x%08lX", *(uint32_t*)&rte_monitor_data.cell_V[cell_id]);
        if (cell_id != (AFEDRV_CELL_CNT - 1)) {
            off += sprintf(log_data + off, ",");
        }
    }
    off += sprintf(log_data + off, "\r\n");
    DEV_ASSERT(off <= APP_TX_DATA_SIZE);
    CDC_Transmit_FS((uint8_t*)log_data, off);
}
#else
static void logger_send_data(void)
{
    uint32_t timestamp_ms = HAL_GetTick();

    int off = 0;
    off += sprintf(log_data + off, "%lu,", timestamp_ms);
    off += sprintf(log_data + off, "%f,", rte_monitor_data.current_A);
    off += sprintf(log_data + off, "%f,", rte_monitor_data.stack_V);

    for (uint8_t cell_id = 0; cell_id < AFEDRV_CELL_CNT; cell_id++) {
        off += sprintf(log_data + off, "%f", rte_monitor_data.cell_V[cell_id]);
        if (cell_id != (AFEDRV_CELL_CNT - 1)) {
            off += sprintf(log_data + off, ",");
        }
    }
    off += sprintf(log_data + off, "\r\n");
    DEV_ASSERT(off <= APP_TX_DATA_SIZE);
    CDC_Transmit_FS((uint8_t*)log_data, off);
}
#endif    // USE_RAW

void logger_send_data_usb_10ms(void)
{
    logger_send_hdl[logger_sts]();
}
