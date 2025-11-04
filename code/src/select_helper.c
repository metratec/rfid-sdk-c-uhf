/**
 * @file: select_helper.c                                                                          *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-23                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#include <metratec/uhf_reader_sdk.h>
#if MT_UHF_SELECT_HELPER_ARRAY_SIZE
#include <metratec/uhf_reader/intern/common.h>
#include <metratec/uhf_reader/public/select_helper.h>

uint8_t select_buffer[MT_UHF_SELECT_HELPER_ARRAY_SIZE][MT_UHF_SELECT_DATA_MAX_LEN];
struct mt_uhf_select_element element_buffer[MT_UHF_SELECT_HELPER_ARRAY_SIZE];

struct mt_uhf_select_data _data;

struct mt_uhf_select_data *mt_uhf_select_build(const uint8_t      *epc_data,
                                               size_t              epc_len,
                                               const uint8_t      *tid_data,
                                               size_t              tid_len,
                                               enum mt_uhf_session session)
{
    if (session > mt_uhf_session_max || session <= mt_uhf_session_unknown)
        return NULL;
    if ((epc_len && !epc_data) || (tid_len && !tid_data))
        return NULL;

    size_t chunks = ((epc_len + MT_UHF_SELECT_DATA_MAX_LEN - 1) / MT_UHF_SELECT_DATA_MAX_LEN);
    chunks += ((tid_len + MT_UHF_SELECT_DATA_MAX_LEN - 1) / MT_UHF_SELECT_DATA_MAX_LEN);
    if (chunks > MT_UHF_SELECT_HELPER_ARRAY_SIZE)
        return NULL;
    _data.elements = element_buffer;
    _data.session  = session;

    if (!chunks) {
        //no masking
        element_buffer->bank               = mt_uhf_mem_bank_EPC;
        element_buffer->data               = NULL;
        element_buffer->data_len_bit       = 0;
        element_buffer->target_address_bit = 0;
        _data.select_element_count         = 1;
        return &_data;
    }

    for (_data.select_element_count = 0; _data.select_element_count < chunks;
         _data.select_element_count++)
    {
        if (tid_len) {
            struct mt_uhf_select_element *e = &element_buffer[_data.select_element_count];
            e->bank                         = mt_uhf_mem_bank_TID;
            unsigned len                    = min(tid_len, MT_UHF_SELECT_DATA_MAX_LEN);
            unsigned offset                 = tid_len - len;
            memcpy(e->data, tid_data + offset, len);
            e->data_len_bit       = 8 * len;
            e->target_address_bit = 8 * offset;
            tid_len -= len;
        } else if (epc_len) {
            struct mt_uhf_select_element *e = &element_buffer[_data.select_element_count];
            e->bank                         = mt_uhf_mem_bank_EPC;
            unsigned len                    = min(epc_len, MT_UHF_SELECT_DATA_MAX_LEN);
            unsigned offset                 = epc_len - len;
            memcpy(e->data, epc_data + offset, len);
            e->data_len_bit       = 8 * len;
            e->target_address_bit = 8 * offset;
            epc_len -= len;
        } else //should never happen
        {
            mt_rfid_reader_assert(0, "Select chunk computation failed");
            return NULL;
        }
    }
    return &_data;
}
#endif