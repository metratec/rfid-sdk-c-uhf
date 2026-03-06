/**
 * @file: posix_serial.c                                                                           *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-16                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

//METRATEC SDK
#include <metratec/uhf_reader/hal.h>

//Basics
#include "interface.h"

//C
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//POSIX
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

#ifndef __USE_POSIX
#warning No posix environment
#endif

static void _serial_cb(int status);

struct serial {
    int port;
} static serial = { .port = -1 };

int comm_start(char *port_name, size_t rx_time)
{
    if (!port_name || rx_time >= 1000)
        return -EINVAL;
    int ret = open(port_name, O_RDWR | O_NONBLOCK | O_NOCTTY);
    if (ret < 0)
        return ret;
    serial.port = ret;
    struct termios tty;
    ret = tcgetattr(serial.port, &tty);
    if (ret != 0)
        return ret;

    tty.c_iflag     = IGNPAR;
    tty.c_oflag     = 0;
    tty.c_lflag     = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = (rx_time / 100);
    cfmakeraw(&tty);
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_cflag &= ~CSTOPB;
#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
#endif

    if ((ret = cfsetispeed(&tty, B115200))) {
        printf("Setting input baudrate failed: %i - %s\n", errno, strerror(errno));
        exit(ret);
    }
    if ((ret = cfsetospeed(&tty, B115200))) {
        printf("Setting output baudrate failed: %i - %s\n", errno, strerror(errno));
        exit(ret);
    }
    ret = tcsetattr(serial.port, TCSANOW, &tty);
    if (ret) {
        printf("tcsetattr failed: %i - %s\n", errno, strerror(errno));
        exit(ret);
    }
    ret = tcflush(serial.port, TCIOFLUSH);
    if (ret) {
        printf("tcflush failed: %i - %s\n", errno, strerror(errno));
        exit(ret);
    }
    return 0;
}

void comm_stop(void)
{
    if (serial.port >= 0)
        close(serial.port);
    serial.port = -1;
}

mt_uhf_errorcode_t comm_update(void)
{
    if (serial.port < 0)
        return mt_uhf_errorcode_not_available;
    size_t space = mt_rfid_reader_rx_remaining_empty();
    if (space == 0)
        return mt_uhf_errorcode_memory_full;
    char   buf[128];
    size_t len = (space >= sizeof(buf)) ? sizeof(buf) : space;

    for (int i = 0; i < 5; i++) {
        ssize_t ret = read(serial.port, buf, len);
        if (ret < 0) {
            if (errno == EBUSY || errno == EWOULDBLOCK || errno == EAGAIN || errno == ETIMEDOUT)
                return 0;
            if (errno == EINTR)
                continue;
            break;
        }
        //success
        if (ret > mt_uhf_errorcode_success_max)
            return mt_uhf_errorcode_range;

        mt_uhf_errorcode_t result = mt_rfid_reader_rx(buf, ret);
        if (result < 0) {
            printf("Read data can't be put into sdk buffer, tried %zu, returned %i -> '%s'\r",
                   ret,
                   result,
                   mt_uhf_error2string(result));
            return result;
        }
        return result;
    }
    exit(-ECONNABORTED);
    return mt_uhf_errorcode_general_fault;
}

mt_uhf_errorcode_t mt_rfid_reader_tx(const uint8_t *data, size_t data_len)
{
    for (int i = 0; i < 5; i++) {
        ssize_t ret = write(serial.port, data, data_len);
        if (ret >= 0) {
            //success
            if (ret > mt_uhf_errorcode_success_max)
                return mt_uhf_errorcode_range;
            return ret;
        }
        if (errno == -EINTR)
            continue;
        if (errno == -EBUSY || errno == -EWOULDBLOCK || errno == -EAGAIN || errno == -ETIMEDOUT)
            return 0;
        break;
    }
    return mt_uhf_errorcode_general_fault;
}
