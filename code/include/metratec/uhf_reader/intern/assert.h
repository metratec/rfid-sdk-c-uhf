/**
 * @file: assert.h                                                                                 *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-28                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#ifndef MT_UHF_SDK_INTERN_ASSERT_H
#define MT_UHF_SDK_INTERN_ASSERT_H

#include <metratec/uhf_reader/intern/common.h>

#if MT_UHF_USE_ASSERT
#include <stdio.h>
#include <string.h>

extern const char *mt_rfid_reader_assert_base_text;
extern const char *mt_rfid_reader_assert_no_message;
extern void        mt_rfid_reader_assert_log(char *message);
#define MT_RFID_ASSERT_CODE_TO_STRING(x) #x
#define mt_rfid_reader_assert(test, usr_message)                                          \
    do {                                                                                  \
        if (test)                                                                         \
            break;                                                                        \
        size_t usr_msg_len = (usr_message ? (strlen(usr_message)) : 0);                   \
        char   message[256 + usr_msg_len];                                                \
        (void)snprintf(message,                                                           \
                       sizeof(message),                                                   \
                       mt_rfid_reader_assert_base_text,                                   \
                       MT_RFID_ASSERT_CODE_TO_STRING(test),                               \
                       __LINE__,                                                          \
                       __FILE__,                                                          \
                       (usr_msg_len ? (usr_message) : mt_rfid_reader_assert_no_message)); \
        mt_rfid_reader_assert_log(message);                                               \
    } while (0)
#else
#define mt_rfid_reader_assert(test, usr_message) \
    do {                                         \
    } while (0)
#endif

#endif //include guard