/**
 * @file: posix_interface.c                                                                        *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-23                                                                        *
 * Author: Martin Koehler & Nils Harder                                                            *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#include <interface.h>

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

volatile static sig_atomic_t abort_request = 0;
static void                  abort_signal_handler(int signo)
{
    (void)signo;
    fprintf(stderr, "Abort requested\n");
    abort_request = 1;
}
int abort_init(void)
{
    struct sigaction sa = { 0 };
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = abort_signal_handler;

    if (sigaction(SIGTERM, &sa, NULL) < 0)
        return -1;
    if (sigaction(SIGINT, &sa, NULL) < 0)
        return -1;

    return 0;
}

int interface_init(void)
{
    srand(time(NULL));
    return abort_init();
}

bool abort_requested(void)
{
    return abort_request != 0;
}

void mt_rfid_reader_assert_log(char *message)
{
    fprintf(stderr, "%s \n", message);
}

uint32_t mt_rfid_reader_get_time(void)
{
    static uint32_t start = 0;
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    uint32_t now = (t.tv_nsec / 1000000 + t.tv_sec * 1000);
    if (!start)
        start = now;
    now -= start;
    return now;
}

void mt_cmd_wait(uint32_t ms)
{
    uint32_t start = mt_rfid_reader_get_time();
    while (ms) {
        const struct timespec wait = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000 };
        int                   ret  = nanosleep(&wait, NULL);
        if (ret == 0)
            return;
        uint32_t now  = mt_rfid_reader_get_time();
        uint32_t done = now - start;
        if (done >= ms)
            break;
        start = now;
        ms -= done;
    }
}
