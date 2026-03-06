/**
 * @file: rw.c                                                                                     *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-30                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */
#include <stdio.h>
#include <string.h>

#include <metratec/uhf_reader_sdk.h>

//sdk needs to come first to include the settings
#include <metratec/uhf_reader/intern/reader.h>

DEFINE_PREFIX(READ);
DEFINE_PREFIX(WRT);
// DEFINE_PREFIX(PWD);
// DEFINE_PREFIX(LCK);
// DEFINE_PREFIX(ULCK);
// DEFINE_PREFIX(PLCK);
// DEFINE_PREFIX(BLCK);
// DEFINE_PREFIX(KILL);
// DEFINE_PREFIX(SEL);

enum mt_uhf_rw_error_id {
    mt_uhf_rw_error_type_no_error = 0,
    mt_uhf_rw_error_type_start    = mt_uhf_rw_error_type_no_error,
    mt_uhf_rw_error_type_general_error,
    mt_uhf_rw_error_type_locked,
    mt_uhf_rw_error_type_power,
    mt_uhf_rw_error_type_memory_overrun,
    mt_uhf_rw_error_type_length
};

const char *mt_uhf_rw_error_names[mt_uhf_rw_error_type_length] = { "OK",
                                                                   "ERROR",
                                                                   "MEMORY LOCKED",
                                                                   "INSUFFICIENT POWER",
                                                                   "MEMORY OVERRUN" };

enum mt_uhf_rw_error_id mt_uhf_rw_get_error_id_from_name(char *name)
{
    enum mt_uhf_rw_error_id id;
    for (id = mt_uhf_rw_error_type_start; id < mt_uhf_rw_error_type_length; id++)
        if (!strcmp(mt_uhf_rw_error_names[id], name))
            break;
    return id;
}
const char *mt_uhf_rw_get_error_name_from_id(enum mt_uhf_rw_error_id id)
{
    if (id >= mt_uhf_rw_error_type_length)
        return NULL;
    return mt_uhf_rw_error_names[id];
}
struct rw_usr_data {
    struct mt_uhf_buffer     *answer_buffer;
    struct mt_uhf_buffer_epc *epc_buffer;
    unsigned                  size;
    unsigned                  found_no_error;
    unsigned                  error;
};

static mt_uhf_errorcode_t _read_data_cb(const char *prefix, char *data, bool finalized)
{
    //Input testing
    mt_rfid_reader_assert(data && prefix, "Missing data or prefix");
    mt_rfid_reader_assert(!strcmp(prefix_READ, prefix), "Wrong prefix");
    struct rw_usr_data *target = reader.cmd.usr_data;
    mt_rfid_reader_assert(target && target->size && target->answer_buffer, "No data");
    if (!data || !prefix)
        return mt_uhf_errorcode_format_fault;

    //Test for no tags found case
    if (!strcmp(data, "<NO TAGS FOUND>")) {
        mt_rfid_reader_assert(target->found_no_error == 0, "No tags found so fill should be zero");
        mt_rfid_reader_assert(target->error == 0, "No tags found so error should be zero");
        return mt_uhf_errorcode_success;
    }

    //tag found, get answer elements
    char *epc   = data;
    char *colon = strchr(epc, ',');
    if (!colon)
        return mt_uhf_errorcode_format_fault;
    size_t epc_char_len = colon - epc;
    char  *result       = colon + 1;
    colon               = strchr(result, ',');
    if (colon)
        *colon = '\0'; //make it a string for parsing function
    //Without colon there is no data field (result should contain the error)
    char  *read_data          = colon ? colon + 1 : NULL;
    size_t read_data_char_len = colon ? strlen(read_data) : 0;
    if (read_data && strchr(result, ','))
        return mt_uhf_errorcode_format_fault;

    //Data parsed, check valid type

    // Check EPC, needs to fit into buffer, is hex chars and a multiple of 4 of them as it consists of 16 bit words
    size_t epc_byte_len = epc_char_len / 2;
    if (epc_char_len % 4 || epc_byte_len > sizeof(target->epc_buffer->data))
        return mt_uhf_errorcode_format_fault;
    if (!mt_parse_check_hex(data, epc_char_len))
        return mt_uhf_errorcode_format_fault;
    //Check Data, needs to fit into buffer, is hex chars and multiple of 2 as its bytes (8 bits)
    if (read_data_char_len % 2)
        return mt_uhf_errorcode_format_fault;
    size_t read_data_byte_len = read_data_char_len / 2;
    if (read_data_byte_len > target->answer_buffer->size)
        return mt_uhf_errorcode_memory_full;
    if (!mt_parse_check_hex(read_data, read_data_char_len))
        return mt_uhf_errorcode_format_fault;
    //Check / evaluate result string
    enum mt_uhf_rw_error_id result_id = mt_uhf_rw_get_error_id_from_name(result);
    if (result_id >= mt_uhf_rw_error_type_length) //unknown string
        return mt_uhf_errorcode_format_fault;

    //everything is parsed or parsable, no data errors
    if (result_id != mt_uhf_rw_error_type_no_error) {
        mt_rfid_reader_assert(read_data_char_len == 0,
                              "If there is an error there should be no data");
        target->error++;
        return mt_uhf_errorcode_success; //read errors are valid
    }
    if (!read_data) //no error but also no data
        return mt_uhf_errorcode_format_fault;

    //Check space for data
    if (target->found_no_error >= target->size) { //buffer full
        target->found_no_error++;                 //One more tag found
        return mt_uhf_errorcode_success;          //but no space left to save it
    }

    //Space available
    if (target->epc_buffer) { //epc is optional
        struct mt_uhf_buffer_epc *e = target->epc_buffer + target->found_no_error;
        e->fill                     = epc_byte_len;
        mt_uhf_errorcode_t ret =
            mt_parse_hex_array_to_bytes(epc, epc_char_len, false, e->data, epc_byte_len);
        if (ret < mt_uhf_errorcode_success)
            return ret;
    }
    struct mt_uhf_buffer *a = target->answer_buffer + target->found_no_error;
    a->fill                 = read_data_byte_len;
    mt_uhf_errorcode_t ret  = mt_parse_hex_array_to_bytes(
        read_data, read_data_char_len, false, a->data, read_data_byte_len);
    if (ret < mt_uhf_errorcode_success)
        return ret;
    target->found_no_error++;
    return mt_uhf_errorcode_success;
}

mt_uhf_errorcode_t mt_uhf_read_data(enum mt_uhf_mem_bank      bank,
                                    unsigned                  len,
                                    unsigned                  offset,
                                    struct mt_uhf_buffer     *answer_buffer_data,
                                    struct mt_uhf_buffer_epc *answer_buffer_epc,
                                    unsigned                  answer_buffer_count,
                                    struct mt_uhf_buffer_epc *mask,
                                    unsigned                 *error_count,
                                    uint32_t                  timeout_ms)
{
    if (answer_buffer_data == NULL || answer_buffer_count == 0 || len == 0 || len > 16)
        return mt_uhf_errorcode_invalid_parameter;
    if (offset & 1)
        return mt_uhf_errorcode_invalid_parameter;
    if (bank >= mt_uhf_mem_bank_length || bank == mt_uhf_mem_bank_OFF)
        return mt_uhf_errorcode_invalid_parameter;
    for (int i = 0; i < answer_buffer_count; i++) {
        if (answer_buffer_data[i].size < len)
            return mt_uhf_errorcode_invalid_parameter;
        answer_buffer_data[i].fill = 0;
        if (answer_buffer_epc)
            answer_buffer_epc[i].fill = 0;
    }

    char cmd[32 + 64];
    int  retp =
        snprintf(cmd, sizeof(cmd), "AT+READ=%s,%u,%u", mt_uhf_get_membank_name(bank), offset, len);
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_no_buffer;
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;
    if (mask) {
        size_t mask_len = min(mask->fill, 16);
        if (sizeof(cmd) < retp + 2 * mask_len + 2)
            return mt_uhf_errorcode_no_buffer;
        //add epc buffer
        cmd[retp++] = ',';
        mt_print_hex(mask->data, mask_len, cmd + retp);
        retp += 2 * mask_len;
    }
    cmd[retp++] = '\0';

    struct rw_usr_data usr_data = {
        .answer_buffer  = answer_buffer_data,
        .epc_buffer     = answer_buffer_epc,
        .size           = answer_buffer_count,
        .found_no_error = 0,
        .error          = 0,
    };
    mt_uhf_errorcode_t ret = mt_uhf_get_data(cmd, _read_data_cb, &usr_data, 0);
    if (ret < mt_uhf_errorcode_success)
        return ret;
    if (error_count)
        *error_count = usr_data.error;
    return usr_data.found_no_error;
}

static int _write_data_cb(const char *prefix, char *data, bool finalized)
{
    //Input testing
    mt_rfid_reader_assert(data && prefix, "Missing data or prefix");
    mt_rfid_reader_assert(!strcmp(prefix_WRT, prefix), "Wrong prefix");
    struct rw_usr_data *target = reader.cmd.usr_data;
    mt_rfid_reader_assert(target && !target->answer_buffer,
                          "No user data or unexpected answer buffer in write");
    mt_rfid_reader_assert(target && (!target->epc_buffer == !target->size),
                          "Size and pointer mismatch");
    if (!data || !prefix)
        return mt_uhf_errorcode_format_fault;

    //Test for no tags found case
    if (!strcmp(data, "<NO TAGS FOUND>")) {
        mt_rfid_reader_assert(target->found_no_error == 0, "No tags found so fill should be zero");
        mt_rfid_reader_assert(target->error == 0, "No tags found so error should be zero");
        return mt_uhf_errorcode_success;
    }

    //tag found, get answer elements, EPC + Result
    char *epc   = data;
    char *colon = strchr(epc, ',');
    if (!colon)
        return mt_uhf_errorcode_format_fault;
    size_t epc_char_len = colon - epc;
    char  *result       = colon + 1;
    if (strchr(result, ','))
        return mt_uhf_errorcode_format_fault;

    // Check EPC, needs to fit into buffer, is hex chars and a multiple of 4 of them as it consists of 16 bit words
    size_t epc_byte_len = epc_char_len / 2;
    if (epc_char_len % 4 || epc_byte_len > sizeof(target->epc_buffer->data))
        return mt_uhf_errorcode_format_fault;
    if (!mt_parse_check_hex(data, epc_char_len))
        return mt_uhf_errorcode_format_fault;

    //Check / evaluate result string
    enum mt_uhf_rw_error_id result_id = mt_uhf_rw_get_error_id_from_name(result);
    if (result_id >= mt_uhf_rw_error_type_length) //unknown string
        return mt_uhf_errorcode_format_fault;

    //everything is parsed or parsable, no data errors
    if (result_id != mt_uhf_rw_error_type_no_error) {
        target->error++;
        return mt_uhf_errorcode_success; //read errors are valid
    }

    //Check space for data
    if (target->found_no_error >= target->size) { //buffer full
        target->found_no_error++;                 //One more tag found
        return mt_uhf_errorcode_success;          //but no space left to save it
    }

    //Space available
    if (target->epc_buffer) { //epc is optional
        struct mt_uhf_buffer_epc *e = target->epc_buffer + target->found_no_error;
        e->fill                     = epc_byte_len;
        mt_uhf_errorcode_t ret =
            mt_parse_hex_array_to_bytes(epc, epc_char_len, false, e->data, epc_byte_len);
        if (ret < mt_uhf_errorcode_success)
            return ret;
    }
    target->found_no_error++;
    return mt_uhf_errorcode_success;
}

int mt_uhf_write_data(enum mt_uhf_mem_bank      bank,
                      uint8_t                  *data,
                      unsigned                  data_len,
                      uint32_t                  offset,
                      struct mt_uhf_buffer_epc *answer_buffer_epc,
                      unsigned                  answer_buffer_count,
                      struct mt_uhf_buffer_epc *mask,
                      unsigned                 *error_count,
                      uint32_t                  timeout_ms)
{
    if (!data || data_len == 0 || data_len > 16 || data_len & 1 || offset & 1)
        return mt_uhf_errorcode_invalid_parameter;
    if (bank >= mt_uhf_mem_bank_length || bank == mt_uhf_mem_bank_OFF)
        return mt_uhf_errorcode_invalid_parameter;
    if (answer_buffer_count && !answer_buffer_epc)
        return mt_uhf_errorcode_invalid_parameter;

    char cmd[32 + 32 + 32]; //32 for command, 32 for data, 64 for mask
    int  retp = snprintf(cmd, sizeof(cmd), "AT+WRT=%s,%u,", mt_uhf_get_membank_name(bank), offset);
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_no_buffer;
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;
    //Add data
    if (retp + 2 * data_len + 1 > sizeof(cmd))
        return mt_uhf_errorcode_no_buffer;
    mt_print_hex(data, data_len, cmd + retp);
    retp += 2 * data_len;

    //Add mask if provided
    if (mask && mask->fill) {
        size_t mask_len = min(mask->fill, 16);
        if (sizeof(cmd) < retp + 2 * mask_len + 2)
            return mt_uhf_errorcode_no_buffer;
        cmd[retp++] = ',';
        mt_print_hex(mask->data, mask_len, cmd + retp);
        retp += 2 * mask_len;
    }
    cmd[retp++] = '\0';

    struct rw_usr_data usr_data = {
        .answer_buffer  = NULL,
        .epc_buffer     = answer_buffer_epc,
        .size           = answer_buffer_count,
        .found_no_error = 0,
        .error          = 0,
    };
    mt_uhf_errorcode_t ret = mt_uhf_get_data(cmd, _write_data_cb, &usr_data, timeout_ms);
    if (ret < mt_uhf_errorcode_success)
        return ret;
    if (error_count)
        *error_count = usr_data.error;
    return usr_data.found_no_error;
}
