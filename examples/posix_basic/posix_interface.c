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

#include <posix_interface.h>

volatile bool running = true;

// void signal_handler_QUIT(int status)
// {
//     running = false;
// }

int posix_interface_init(void)
{
    // struct sigaction saquit;
    // saquit.sa_handler  = signal_handler_QUIT;
    // saquit.sa_flags    = 0;
    // saquit.sa_restorer = NULL;
    // //sigaction(SIGINT, &saquit, NULL);
    return EXIT_SUCCESS;
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
