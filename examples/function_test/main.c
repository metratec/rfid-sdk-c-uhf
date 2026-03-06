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

//Tester
#include "testing_functions.h"

//Metratec SDK
#include <metratec/uhf_reader_sdk.h>

bool tag_cb(struct mt_uhf_gen2_tag *tagp)
{
    static unsigned tag_count = 0;
    if (!tagp) {
        printf("Round finished, got %u tags\n", tag_count);
        tag_count = 0;
        return false; //No data, no buffering
    }
    printf("Tag found, EPC: ");
    for (int i = 0; i < tagp->epc.fill; i++) {
        if (i & 3 == 0)
            putchar(' ');
        printf("%02X", tagp->epc.data[i]);
    }
    if (tagp->tid.fill) {
        printf(", TID:");
        for (int i = 0; i < tagp->tid.fill; i++) {
            if (i & 3 == 0)
                putchar(' ');
            printf("%02X", tagp->tid.data[i]);
        }
    }
    printf(", %u times\n", tagp->count);
    printf("RSSI: %i, Phase: %i / %i\n", tagp->rssi, tagp->phase[0], tagp->phase[1]);
    tag_count++;
    return true;
}

static mt_uhf_errorcode_t _unexpected_frame_cb(const char *frame_data)
{
    printf("Received an unknown frame:\n%s\n", frame_data);
    return mt_uhf_errorcode_no_data_available;
}

//Number of tags buffered
static struct mt_uhf_gen2_tag  tags[64];
struct mt_uhf_inventory_buffer inv_buffer = {
    .antenna = 0,
    .tags    = { .buffer = tags, .size = ARRAY_SIZE(tags), .fill = 0 }
};

void round_done(struct mt_uhf_inventory_buffer *buf)
{
    printf("Round done with %u tags:\n", buf->tags.fill);
    for (int t = 0; t < buf->tags.fill; t++) {
        printf("EPC (%u bytes) found %u times: ",
               buf->tags.buffer[t].epc.fill,
               buf->tags.buffer[t].count);
        for (int b = 0; b < buf->tags.buffer[t].epc.fill; b += 2)
            printf(
                "%02X%02X ", buf->tags.buffer[t].epc.data[b], buf->tags.buffer[t].epc.data[b + 1]);
        putchar('\n');
    }
}

char *comm_port_name = NULL;

mt_uhf_errorcode_t connection(bool connect)
{
    if (!connect) {
        comm_stop();
        return mt_uhf_errorcode_success;
    }

    int ret = comm_start(comm_port_name, 5);
    if (ret) {
        fprintf(stderr,
                "Couldn't open %s, ret: %i - %s, Errno: %i - %s\n",
                comm_port_name,
                ret,
                strerror(-ret),
                errno,
                strerror(-errno));
        comm_stop();
        return mt_uhf_errorcode_general_fault;
    }
    return mt_uhf_errorcode_success;
}

mt_uhf_errorcode_t reset_done_cb(void)
{
    printf("Reset done called\n");
    (void)connection(false);
    uint32_t start           = mt_rfid_reader_get_time();
    uint32_t reset_wait_time = 1000;
    if (!strcmp(MT_DEVICE_TYPE_SET, "DESKID_UHF_V2_ETSI") ||
        !strcmp(MT_DEVICE_TYPE_SET, "DESKID_UHF_V2_FCC"))
        reset_wait_time = 5000;

    while (1) {
        if (abort_requested())
            return mt_uhf_errorcode_general_fault;
        uint32_t done = mt_rfid_reader_get_time() - start;
        if (done >= reset_wait_time)
            break;
        mt_cmd_wait(min(reset_wait_time - done, 100));
    }
    return connection(true);
}

int main(int argc, char *argv[])
{
    const const uint8_t q_min = 2, q = 4, q_max = 6;
    const const uint8_t antenna_power[] = { MT_UHF_POWER_MAX - 5 };
    const uint32_t      test_time_INVR = 15000, test_time_CINVR = 10000, test_time_CINV = 10000;
    enum mt_uhf_region  region;
    if (!strcmp(MT_DEVICE_TYPE_SET, "QRG2_ETSI") ||
        !strcmp(MT_DEVICE_TYPE_SET, "DESKID_UHF_V2_ETSI") || !strcmp(MT_DEVICE_TYPE_SET, "PLRM"))
        region = mt_uhf_region_etsi;
    else
        region = mt_uhf_region_fcc;

    const enum mt_uhf_rf_mode mode    = mt_uhf_rf_mode_285;
    const enum mt_uhf_session session = mt_uhf_session_s0;

#if MAX_ANTENNAS_MUXED
    uint8_t mux_test[MAX_ANTENNAS_MUXED];
    for (int i = 0; i < MAX_ANTENNAS_MUXED; i++)
        mux_test[i] = i + 1;
#else
    const uint8_t *mux_test = NULL;
#endif

    mt_uhf_errorcode_t ret = mt_uhf_errorcode_invalid_parameter;
    if (argc != 2) {
        fprintf(stderr, "Please provide path to serial port! \n");
        goto exit;
    }
    comm_port_name = argv[1];
    if ((ret = connection(true)))
        goto exit;

    {
        int ret_init = interface_init();
        if (ret_init) {
            fprintf(stderr,
                    "Interface init failed with error %d: %s \n",
                    -ret_init,
                    strerror(ret_init));
            ret = mt_uhf_errorcode_general_fault;
            goto exit;
        }
    }

    if ((ret = mt_uhf_init(_unexpected_frame_cb, comm_update, NULL, reset_done_cb))) {
        printf("Initialisation returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        goto exit;
    }
    mt_uhf_timeout_set(15000, 30000);
    if (abort_requested() || !test_device_info())
        goto exit;
    if (abort_requested() || !test_outputs())
        goto exit;
    if (abort_requested() || !test_inputs(/**No inputs on PLRM*/))
        goto exit;
    if (abort_requested() || !test_region_mode(region, mode))
        goto exit;
    if (abort_requested() || !test_session(session))
        goto exit;
    if (abort_requested() || !test_mux_antenna(mux_test, mux_test ? ARRAY_SIZE(mux_test) : 0))
        goto exit;
    if (abort_requested())
        goto exit;
    ret = mt_uhf_set_power(antenna_power, sizeof(antenna_power));
    if (ret < 0) {
        printf("Power setting returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        goto exit;
    }
    if (abort_requested())
        goto exit;
    if ((ret = mt_uhf_set_q_value(q, q_min, q_max))) {
        printf("Q setting returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        goto exit;
    }
    if (abort_requested())
        goto exit;
    ret = mt_uhf_set_inventory_settings(false,
                                        true,
                                        true,
                                        false,
                                        true,
                                        mt_uhf_gen2_inventory_selecting_all,
                                        mt_uhf_gen2_inventory_target_A,
                                        -100);
    if (ret) {
        printf("Inventory setting returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        goto exit;
    }
    if (abort_requested())
        goto exit;
    if (!test_masking())
        goto exit;
    /**
     * Preparation and reader settings tests done
     */

    //Normal inventory
    if (abort_requested())
        goto exit;
    ret = mt_uhf_inventory(tag_cb, NULL, 5000);
    if (ret < 0) {
        printf("Inventory returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        goto exit;
    }
    // //Inventory with mux (MINV)
    // ret = mt_uhf_inventory_automux(tag_cb, NULL, 5000);
    // if (ret < 0) {
    //     printf("Mux inventory returned error %d: %s \n", ret, mt_uhf_error2string(ret));
    //     goto exit;
    // }
    //More complex inventories
    if (abort_requested())
        goto exit;
    if (!test_invr(1, &inv_buffer, test_time_INVR))
        goto exit;
    if (abort_requested())
        goto exit;
    if (!test_cinvr(1, &inv_buffer, test_time_CINVR, round_done))
        goto exit;
    if (abort_requested())
        goto exit;
    if (!test_cinv(1, &inv_buffer, test_time_CINV, round_done))
        goto exit;
    // if (abort_requested())
    //     goto exit;
    // if (!test_cminv(0, &inv_buffer, test_time_CINV, round_done))
    //     goto exit;
    if (abort_requested())
        goto exit;
    if (!test_tid())
        goto exit;

    if (inv_buffer.tags.fill) {
        for (unsigned i = 0; i < inv_buffer.tags.fill; i++)
            (void)test_rw_masked(&inv_buffer.tags.buffer[i]);
        (void)mt_uhf_set_inventory_mask(mt_uhf_mem_bank_OFF, 0, NULL, 0);
    } else
        printf("No read / write test as there is no tag in buffer\n");

    printf("-------\nEvery test done once, starting infinite tests in random order\n-------\n\n");

    while (1) {
        if (abort_requested())
            goto exit;
        test_random_function();
    }

exit:
    comm_stop();
    exit(ret >= mt_uhf_errorcode_success ? 0 : -1);
}