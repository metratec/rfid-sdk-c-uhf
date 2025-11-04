/*
 * File: qrg2.h                                                                           *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-16                                                                        *
 * Author: Martin Koehler / Nils Harder                                                            *
 * -----                                                                                           *
 * Copyright (C) 2024                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#pragma once

#ifdef MT_DEVICE_TYPE_SET
#error "Multiple device type includes"
#else
#define MT_DEVICE_TYPE_SET
#endif
#include <stdint.h>

#define MT_UHF_MINIMUM_FW 110

#define MT_UHF_ANTENNA_COUNT 1
#define MT_UHF_GPO_COUNT     0
#define MT_UHF_GPI_COUNT     0
#define MT_UHF_MUX_MAX       0

#ifndef MT_UHF_POWER_MAX
#define MT_UHF_POWER_MAX 9
#endif
#ifndef MT_UHF_POWER_MIN
#define MT_UHF_POWER_MIN 0
#endif

#define MT_DEVICE_MODE_ALLOWED_FOR_REGION                     \
    {                                                         \
        { mt_uhf_rf_mode_103, mt_uhf_region_fcc },            \
        { mt_uhf_rf_mode_120, mt_uhf_region_fcc },            \
        { mt_uhf_rf_mode_244, mt_uhf_region_fcc },            \
        { mt_uhf_rf_mode_302, mt_uhf_region_fcc },            \
        { mt_uhf_rf_mode_323, mt_uhf_region_fcc },            \
        { mt_uhf_rf_mode_345, mt_uhf_region_fcc },            \
        { mt_uhf_rf_mode_222, mt_uhf_region_etsi_lowerband }, \
        { mt_uhf_rf_mode_223, mt_uhf_region_etsi_lowerband }, \
        { mt_uhf_rf_mode_241, mt_uhf_region_etsi_lowerband }, \
        { mt_uhf_rf_mode_344, mt_uhf_region_etsi_upperband }, \
        { mt_uhf_rf_mode_285, mt_uhf_region_all },            \
    }

#define MT_DEVICE_DEFAULT_BAUDRATE 115200

//last colon missing so it can be in the code to look proper
#define MT_DEVICE_VALID_MODES \
    case mt_uhf_rf_mode_222:  \
    case mt_uhf_rf_mode_223:  \
    case mt_uhf_rf_mode_241:  \
    case mt_uhf_rf_mode_244:  \
    case mt_uhf_rf_mode_285

//It's bit field so just add them
#define MT_DEVICE_VALID_REGIONS (mt_uhf_region_etsi)

#define MT_UHF_SELECT_DATA_MAX_LEN 18

#define MT_UHF_RSSI_THRESHOLD_UNSUPPORTED