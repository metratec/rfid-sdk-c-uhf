/**
 * @file: testing_functions.h                                                                      *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-28                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#ifndef MT_UHF_SDK_EXAMPLE_TEST_FUNCTIONS_H
#define MT_UHF_SDK_EXAMPLE_TEST_FUNCTIONS_H

#include <metratec/uhf_reader_sdk.h>

//from test_continious.c
void test_random_function(void);

bool test_masking(void);
bool test_region_mode(enum mt_uhf_region region, enum mt_uhf_rf_mode mode);
bool test_device_info(void);
bool test_outputs(void);
#if MT_UHF_GPI_COUNT
bool test_inputs(mt_uhf_input_states_t *expected);
#else
//intentional leave parameters open so anything put in is valid (and does not get used)
bool test_inputs();
#endif

bool test_session(enum mt_uhf_session session);
bool test_mux_antenna(const uint8_t *antennas, unsigned antenna_count);
bool test_invr(unsigned hbt, struct mt_uhf_inventory_buffer *inv_buffer, unsigned test_time);
bool test_cinvr(unsigned                        hbt_time,
                struct mt_uhf_inventory_buffer *inv_buffer,
                unsigned                        test_time,
                void (*round_done_cb)(struct mt_uhf_inventory_buffer *buf));
bool test_cinv(unsigned                        hbt_time,
               struct mt_uhf_inventory_buffer *inv_buffer,
               unsigned                        test_time,
               void (*round_done_cb)(struct mt_uhf_inventory_buffer *buf));
bool test_cminv(unsigned                        hbt_time,
                struct mt_uhf_inventory_buffer *inv_buffer,
                unsigned                        test_time,
                void (*round_done_cb)(struct mt_uhf_inventory_buffer *buf));
bool test_tid(void);
bool test_rw_masked(struct mt_uhf_gen2_tag *tag);

#endif //include guard