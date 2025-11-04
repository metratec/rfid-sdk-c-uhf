/**
 * @file: main.c                                                                                   *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-24                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

//Posix
#include <posix_interface.h>

//UHF lib
#include <metratec/uhf_reader_sdk.h>

//

#define _POSIX_SOURCE 1 /* POSIX compliant source */

uint32_t      first_time = 0;
int           tags       = 0;
volatile bool running    = true;

void tag_cb(struct mt_uhf_gen2_tag *tagp)
{
    struct mt_uhf_gen2_tag tag;

    memcpy(&tag, tagp, sizeof(tag));
    tags++;
}

void error_cb(int error, char *error_msg)
{
    static int rounds = 0;
    if (error > 0) {
        rounds++;
        uint32_t current_time = mt_rfid_reader_get_time();
        float    duration     = (float)(current_time - first_time) / 1000.0;
        float    rate         = (float)(tags) / duration;
        printf("%d: Round %d on Ant %d finished with %d tags,%f seconds, %f Tags/s\n",
               current_time,
               rounds,
               error,
               tags,
               duration,
               rate);
        tags       = 0;
        first_time = current_time;
    } else {
        fprintf(stderr, "Inventory Error: %s \t- %s\n", strerror(-error), error_msg);
    }
}

int main(int argc, char *argv[])
{
    int ret = -EINVAL;
    if (argc != 2) {
        fprintf(stderr, "Please provide path to serial port! \n");
        goto exit;
    }

    if ((ret = comm_start(argv[1], 5))) {
        printf("Couldn't open %s, Errno %i: %s\n", argv[1], errno, strerror(errno));
        goto exit;
    }
    if ((ret = posix_interface_init()))
        goto exit;

    enum mt_uhf_rf_mode mode = mt_uhf_rf_mode_222;

    const u_int8_t antenna_power[] = { 0 };
    const uint8_t  q               = 2;
    const uint8_t  q_min           = 0;
    const uint8_t  q_max           = 10;

    if ((ret = mt_uhf_init())) {
        printf("Initialisation returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    struct mt_uhf_reader_identification *ident = mt_uhf_get_identification();
    printf("Device FW: %s \n HW: %s \n", ident->fw_name, ident->hw_name);
    int tags   = 0;
    int rounds = 1;

    if ((ret = mt_uhf_set_q_value(q, q_min, q_max))) {
        printf("Q setting returned error %d: %s \n", -ret, strerror(-ret));
        exit(-1);
    }

    ret = mt_uhf_set_power(antenna_power, sizeof(antenna_power));
    if (ret < 0) {
        printf("Power setting returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }

    if ((ret = mt_uhf_set_antenna(4))) {
        printf("Antenna init failed\n");
        goto exit;
    }

    ret = mt_uhf_set_rf_mode(mode);
    if (ret < 0) {
        printf("RF-Mode configuration returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    mt_uhf_set_tag_callback(tag_cb);
    mt_uhf_set_error_callback(error_cb);
    ret = mt_uhf_inventory_start(NULL);
    if (ret < 0) {
        printf("Inventory start returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    first_time = mt_rfid_reader_get_time();
    while (running) {
        ret = mt_rfid_resolve();
        switch (ret) {
        case -EAGAIN:
        case -ENODATA:
        case EXIT_SUCCESS:
            continue;
        default:
            printf("Unexpected error %d from resolve: %s", -ret, strerror(-ret));
        }
    }
    ret = mt_uhf_inventory_stop();
    if (ret < 0) {
        printf("Inventory stop returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
exit:
    comm_stop();
    exit(ret >= EXIT_SUCCESS ? EXIT_SUCCESS : -EFAULT);
}