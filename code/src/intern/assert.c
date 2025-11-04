/**
 * @file: assert.c                                                                                 *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-16                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#include <metratec/uhf_reader/config.h>

#if MT_UHF_USE_ASSERT
const char *mt_rfid_reader_assert_base_text =
    "Assert: Test (%s) failed at line %u in file '%s': %s";
const char *mt_rfid_reader_assert_no_message = "No assert message";
#endif