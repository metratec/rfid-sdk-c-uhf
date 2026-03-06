/**
 * @file: main.c                                                                                   *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-28                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

//C
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Interface
#include <interface.h>
#include <serial.h>

//Metratec
#include <metratec/uhf_reader_sdk.h>

//Settings
const uint8_t             antenna = 1;
const uint8_t             q_min = 2, q = 4, q_max = 6;
const u_int8_t            antenna_power = 9;
const enum mt_uhf_region  region        = mt_uhf_region_etsi;
const enum mt_uhf_rf_mode mode          = mt_uhf_rf_mode_285;
const enum mt_uhf_session session       = mt_uhf_session_s0;

static bool               _cb(struct mt_uhf_gen2_tag *tag);
static mt_uhf_errorcode_t _unexpected_frame_cb(const char *frame_data);

int reset_done_cb(void)
{
    uint32_t start = mt_rfid_reader_get_time();
    while (1) {
        uint32_t done = mt_rfid_reader_get_time() - start;
        if (done >= 2000)
            break;
        mt_cmd_wait(2000 - done);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    //Check parameter
    if (argc == 1) {
        fprintf(stderr, "Please provide a serial port! \n");
        return -EINVAL;
    }
    if (argc > 2) {
        fprintf(stderr, "Please provide only 1 argument (serial port)! \n");
        return -EINVAL;
    }

    //Open serial connection and init posix interface
    {
        printf("Start connection to port %s\n", argv[1]);
        int ret = comm_start(argv[1], 5);
        if (ret) {
            fprintf(stderr,
                    "Couldn't open %s, ret: %i - %s, Errno: %i - %s\n",
                    argv[1],
                    ret,
                    strerror(-ret),
                    errno,
                    strerror(-errno));
            goto exit;
        }
    }

    //Init UHF lib
    mt_uhf_errorcode_t ret = mt_uhf_init(_unexpected_frame_cb, comm_update, NULL, reset_done_cb);
    if (ret == mt_uhf_errorcode_no_valid_device) {
        printf(
            "Firmware version on device is deprecated, please update to Version %04u or higher!\n",
            MT_UHF_MINIMUM_FW);
        goto exit;
    } else if (ret < mt_uhf_errorcode_success) {
        printf("Initialisation returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        goto exit;
    }
    if (abort_requested())
        goto exit;
    struct mt_uhf_reader_identification id;
    ret = mt_uhf_get_identification(&id);
    if (ret < mt_uhf_errorcode_success) {
        printf("Identification returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        goto exit;
    }
    printf("Found device %s %s with Firmware %s %s, starting configuration\n",
           id.hw_name,
           id.hw_rev,
           id.fw_name,
           id.fw_rev);

    if (abort_requested())
        goto exit;
    //Settings for UHF device
    ret = mt_uhf_set_antenna(antenna);

    ret = mt_uhf_set_power(&antenna_power, 1);
    if (ret < 0) {
        printf("Power setting returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        goto exit;
    }

    if (abort_requested())
        goto exit;
    ret = mt_uhf_set_q_value(q, q_min, q_max);
    if (ret) {
        printf("Q setting returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        goto exit;
    }

    if (abort_requested())
        goto exit;
    if ((ret = mt_uhf_set_rf_mode(region, mode))) {
        printf("Setting RF mode returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        return false;
    }

    if (abort_requested())
        goto exit;
    if ((ret = mt_uhf_set_session(session))) {
        printf("Setting invalid RF mode returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        return false;
    }

    if (abort_requested())
        goto exit;
    ret = mt_uhf_set_inventory_settings(false,
                                        false,
                                        false,
                                        false,
                                        false,
                                        mt_uhf_gen2_inventory_selecting_all,
                                        mt_uhf_gen2_inventory_target_A,
                                        -100);
    if (ret) {
        printf("Inventory setting returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        goto exit;
    }
    /** Preparation and reader settings tests done */
    printf("Initialisation finished, reading Tags\n");
    printf("Press CTRL-C to stop reading\n");
    while (1) {
        if (abort_requested())
            goto exit;
        ret = mt_uhf_inventory(_cb, NULL, 5000);
        if (ret < 0) {
            printf("Inventory returned error %d: %s \n", ret, mt_uhf_error2string(ret));
            goto exit;
        }
    }

exit:
    comm_stop();
    exit(ret >= mt_uhf_errorcode_success ? 0 : -1);
}

static bool _cb(struct mt_uhf_gen2_tag *tag)
{
    static unsigned tag_count = 0;
    if (!tag) {
        printf("Round finished, got %u tags\n", tag_count);
        tag_count = 0;
        return false; //No data, no buffering
    }
    printf("Tag found, EPC: ");
    for (int i = 0; i < tag->epc.fill; i++) {
        if ((i & 3) == 0)
            putchar(' ');
        printf("%02X", tag->epc.data[i]);
    }
    if (tag->tid.fill) {
        printf(", TID:");
        for (int i = 0; i < tag->tid.fill; i++) {
            if ((i & 3) == 0)
                putchar(' ');
            printf("%02X", tag->tid.data[i]);
        }
    }
    printf(", %u times", tag->count);
    if (tag->rssi)
        printf(", RSSI: %i", tag->rssi);
    if (tag->phase[0] || tag->phase[1])
        printf(", Phase: %i / %i", tag->phase[0], tag->phase[1]);
    putchar('\n');

    tag_count++;
    return true;
}

static mt_uhf_errorcode_t _unexpected_frame_cb(const char *frame_data)
{
    printf("Received an unknown frame:\n%s\n", frame_data);
    return mt_uhf_errorcode_no_data_available;
}