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

//First get the example functions
#include <posix_interface.h>
#include <testing_functions.h>

//Then the metratec lib header
#include <metratec/uhf_reader_sdk.h>

uint32_t      first_time = 0;
volatile bool running    = true;

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

int unknown_frame_cb(const char *frame_data)
{
    printf("Received an unknown frame:\n%s\n", frame_data);
    return mt_uhf_current_frame_handled();
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

int main(int argc, char *argv[])
{
    const uint8_t         q_min = 2, q = 4, q_max = 8;
    const u_int8_t        antenna_power[] = { 20, 20, 20, 20 };
    const uint32_t        test_time_INVR = 5000, test_time_CINVR = 10000, test_time_CINV = 10000;
    enum mt_uhf_region    region               = mt_uhf_region_etsi;
    enum mt_uhf_rf_mode   mode                 = mt_uhf_rf_mode_285;
    enum mt_uhf_session   session              = mt_uhf_session_s0;
    uint8_t               mux_test[]           = { 4, 4, 4 };
    mt_uhf_input_states_t expected_input_state = { mt_uhf_boolx_false, mt_uhf_boolx_true };

    int ret = -EINVAL;
    if (argc != 2) {
        fprintf(stderr, "Please provide path to serial port! \n");
        goto exit;
    }

    if ((ret = comm_start(argv[1], false, 5))) {
        fprintf(stderr,
                "Couldn't open %s, ret: %i - %s, Errno: %i - %s\n",
                argv[1],
                ret,
                strerror(-ret),
                errno,
                strerror(-errno));
        goto exit;
    }
    if ((ret = posix_interface_init()))
        goto exit;

    if ((ret = mt_uhf_init(&unknown_frame_cb, comm_update, NULL))) {
        printf("Initialisation returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    if (!test_device_info())
        goto exit;
    if (!test_outputs())
        goto exit;
    if (!test_inputs(&expected_input_state))
        goto exit;
    if (!test_region_mode(region, mode))
        goto exit;
    if (!test_session(session))
        goto exit;
    if (!test_mux_antenna(mux_test, ARRAY_SIZE(mux_test)))
        goto exit;

    ret = mt_uhf_set_power(antenna_power, sizeof(antenna_power));
    if (ret < 0) {
        printf("Power setting returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    if ((ret = mt_uhf_set_q_value(q, q_min, q_max))) {
        printf("Q setting returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }

    ret = mt_uhf_set_inventory_settings(false,
                                        true,
                                        true,
                                        false,
                                        true,
                                        mt_uhf_gen2_inventory_selecting_all,
                                        mt_uhf_gen2_inventory_target_A,
                                        -100);
    if (ret) {
        printf("Inventory setting returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    if (!test_masking())
        goto exit;
    /**
     * Preparation and reader settings tests done
     */

    //Normal inventory
    ret = mt_uhf_inventory(tag_cb, NULL, 5000);
    if (ret < 0) {
        printf("Inventory returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    //Inventory with mux (MINV)
    ret = mt_uhf_inventory_automux(tag_cb, NULL, 5000);
    if (ret < 0) {
        printf("Mux inventory returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    //More complex inventories
    if (!test_invr(1, &inv_buffer, test_time_INVR))
        goto exit;
    if (!test_cinvr(1, &inv_buffer, test_time_CINVR, round_done))
        goto exit;
    if (!test_cinv(1, &inv_buffer, test_time_CINV, round_done))
        goto exit;
    if (!test_cminv(0, &inv_buffer, test_time_CINV, round_done))
        goto exit;
    if (!test_tid())
        goto exit;

    //RW test
    ret = mt_uhf_inventory(NULL, &inv_buffer, 5000); //reset the tag list
    if (ret < 0) {
        printf("Inventory returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }

    if (inv_buffer.tags.fill) {
        for (unsigned i = 0; i < inv_buffer.tags.fill; i++) {
            if (!test_rw_masked(&inv_buffer.tags.buffer[i]))
                goto exit;
        }
    } else
        printf("No read / write test as there is no tag in buffer\n");

    /**
     * Inventory functions done
     */

    printf("-------\nTests done\n-------\n\n");

exit:
    comm_stop();
    exit(ret >= EXIT_SUCCESS ? EXIT_SUCCESS : -EFAULT);
}