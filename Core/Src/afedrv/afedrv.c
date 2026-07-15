#include "afedrv_common.h"
#include "util.h"
#include <stdbool.h>
#include "afedrv_monitoring.h"
#include "bcc.h"
#include "rte.h"

bcc_data_t g_bccData;

static bcc_status_t initRegisters(void);
static bcc_status_t clearFaultRegs(void);

void afedrv_init(void)
{
    bcc_status_t bccError;
    g_bccData.drvConfig.drvInstance = 0U;
    g_bccData.drvConfig.commMode = BCC_MODE_SPI;
    g_bccData.drvConfig.devicesCnt = 1U;
    g_bccData.drvConfig.device[0] = BCC_DEVICE_MC33772C;
    g_bccData.drvConfig.cellCnt[0] = 6U;

    /* Precalculate NTC look up table for fast temperature measurement. */
    ntc_config_t ntcConfig;
    ntcConfig.rntc = 6800U;    /* NTC pull-up 6.8kOhm */
    ntcConfig.refTemp = 25U;   /* NTC resistance 10kOhm at 25 degC */
    ntcConfig.refRes = 10000U; /* NTC resistance 10kOhm at 25 degC */
    ntcConfig.beta = 3900U;
    fillNtcTable(&ntcConfig);

    /* Initialize BCC device. */
    bccError = BCC_Init(&g_bccData.drvConfig);
    if (bccError != BCC_STATUS_SUCCESS) {
        UNREACHABLE("BCC_Init");
    }

    /* Initialize BCC device registers. */
    bccError = initRegisters();
    if (bccError != BCC_STATUS_SUCCESS) {
        UNREACHABLE("BCC_Init");
    }

    /* Clear fault registers. */
    bccError = clearFaultRegs();
    if (bccError != BCC_STATUS_SUCCESS) {
        UNREACHABLE("BCC_Init");
    }
}

void afedrv_runnable_10ms(void)
{
    uint8_t cid = BCC_CID_DEV1;
    bcc_status_t error;
    DEV_ASSERT(g_bccData.drvConfig.devicesCnt == 1);
    // for (cid = BCC_CID_DEV1; cid <= g_bccData.drvConfig.devicesCnt; cid++)  // TODO: maybe support multiple AFE in the future
    {
        /* Send NOP command to all nodes in order to prevent communication timeout. */
        sendNops();

        /* Do a measurement and print the measured values. */
        if ((error = doMeasurements(cid)) != BCC_STATUS_SUCCESS) {
            UNREACHABLE("doMeasurements");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION
////////////////////////////////////////////////////////////////////////////////////////////////////

/*!
 * @brief Initializes BCC device registers according to BCC_INIT_CONF.
 * Registers having the wanted content already after POR are not rewritten.
 */
static bcc_status_t initRegisters(void)
{
    uint8_t cid, i;
    bcc_status_t status;

    for (cid = 1; cid <= g_bccData.drvConfig.devicesCnt; cid++) {
        if (g_bccData.drvConfig.device[cid - 1] == BCC_DEVICE_MC33771C) {
            for (i = 0; i < MC33771C_INIT_CONF_REG_CNT; i++) {
                if (s_initRegsMc33771c[i].value != s_initRegsMc33771c[i].defaultVal) {
                    status = BCC_Reg_Write(&g_bccData.drvConfig, (bcc_cid_t)cid, s_initRegsMc33771c[i].address, s_initRegsMc33771c[i].value);
                    if (status != BCC_STATUS_SUCCESS) {
                        return status;
                    }
                }
            }
        } else {
            for (i = 0; i < MC33772C_INIT_CONF_REG_CNT; i++) {
                if (s_initRegsMc33772c[i].value != s_initRegsMc33772c[i].defaultVal) {
                    status = BCC_Reg_Write(&g_bccData.drvConfig, (bcc_cid_t)cid, s_initRegsMc33772c[i].address, s_initRegsMc33772c[i].value);
                    if (status != BCC_STATUS_SUCCESS) {
                        return status;
                    }
                }
            }
        }
    }

    return BCC_STATUS_SUCCESS;
}

/*!
 * @brief Clears all fault registers of BCC devices.
 */
static bcc_status_t clearFaultRegs()
{
    uint8_t cid;
    bcc_status_t status;

    for (cid = 1; cid <= g_bccData.drvConfig.devicesCnt; cid++) {
        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_CELL_OV);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }

        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_CELL_UV);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }

        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_CB_OPEN);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }

        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_CB_SHORT);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }

        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_GPIO_STATUS);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }

        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_AN_OT_UT);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }

        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_GPIO_SHORT);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }

        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_COMM);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }

        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_FAULT1);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }

        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_FAULT2);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }

        status = BCC_Fault_ClearStatus(&g_bccData.drvConfig, (bcc_cid_t)cid, BCC_FS_FAULT3);
        if (status != BCC_STATUS_SUCCESS) {
            return status;
        }
    }

    return BCC_STATUS_SUCCESS;
}
