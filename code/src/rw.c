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
    unsigned                  fill;
    unsigned                  error;
};
static int _read_data_cb(const char *prefix, char *data, bool finalized)
{
    mt_rfid_reader_assert(data && prefix, "Missing data or prefix");
    mt_rfid_reader_assert(!strcmp(prefix_READ, prefix), "Wrong prefix");
    struct rw_usr_data *ud = reader.cmd.usr_data;
    mt_rfid_reader_assert(ud && ud->size && ud->answer_buffer, "No data");

    if (!data)
        return -EBADMSG;
    if (!strcmp(data, "<NO TAGS FOUND>")) {
        mt_rfid_reader_assert(ud->fill == 0, "No tags found so fill should be zero");
        mt_rfid_reader_assert(ud->error == 0, "No tags found so error should be zero");
        return EXIT_SUCCESS;
    }

    char *epc   = data;
    char *colon = strchr(epc, ',');
    if (!colon)
        return -EBADMSG;
    size_t epc_len = colon - epc;
    char  *result  = colon + 1;
    colon          = strchr(result, ',');
    if (colon)
        *colon = '\0'; //make it a string for parsing function
    //Without colon there is no data field (result should contain the error)
    char  *read_data     = colon ? colon + 1 : NULL;
    size_t read_data_len = colon ? strlen(read_data) : 0;

    //Data parsed, check valid type
    if (epc_len & 0x03 || read_data_len & 0x01)
        return -EBADMSG;
    if (!mt_parse_check_hex(epc, epc_len) || !mt_parse_check_hex(read_data, read_data_len))
        return -EBADMSG;

    //Evaluate result string
    enum mt_uhf_rw_error_id result_id = mt_uhf_rw_get_error_id_from_name(result);
    if (result_id >= mt_uhf_rw_error_type_length)
        return -EBADMSG;
    //everything is parsed or parsable, no data errors
    if (result_id != mt_uhf_rw_error_type_no_error) {
        mt_rfid_reader_assert(read_data_len == 0, "If there is an error there should be no data");
        ud->error++;
        return EXIT_SUCCESS;
    }

    if (read_data_len > ud->answer_buffer[ud->fill].size * 2 ||
        epc_len > MT_UHF_GEN2_MAX_EPC_BYTES * 2)
        return -ENOBUFS;

    if (ud->fill < ud->size) {
        if (ud->epc_buffer) {
            struct mt_uhf_buffer_epc *e = ud->epc_buffer + ud->fill;
            e->fill                     = epc_len / 2;
            mt_parse_hex_array_to_bytes(epc, epc_len, false, e->data, e->fill);
        }
        struct mt_uhf_buffer *a = ud->answer_buffer;
        a->fill                 = read_data_len / 2;
        mt_parse_hex_array_to_bytes(read_data, read_data_len, false, a->data, a->fill);
    }
    ud->fill++;
    return EXIT_SUCCESS;
}

int mt_uhf_read_data(enum mt_uhf_mem_bank      bank,
                     unsigned                  len,
                     unsigned                  offset,
                     struct mt_uhf_buffer     *answer_buffer_data,
                     struct mt_uhf_buffer_epc *answer_buffer_epc,
                     unsigned                  answer_buffer_count,
                     struct mt_uhf_buffer_epc *mask,
                     unsigned                 *error_count)
{
    if (answer_buffer_data == NULL || answer_buffer_count == 0 || len == 0 || len > 16)
        return -EINVAL;
    if (offset & 1)
        return -EINVAL;
    if (bank >= mt_uhf_mem_bank_length || bank == mt_uhf_mem_bank_OFF)
        return -EINVAL;
    for (int i = 0; i < answer_buffer_count; i++) {
        if (answer_buffer_data[i].size < len)
            return -EINVAL;
        answer_buffer_data[i].fill = 0;
        if (answer_buffer_epc)
            answer_buffer_epc[i].fill = 0;
    }

    int  ret = EXIT_SUCCESS;
    char cmd[32 + 64];
    ret =
        snprintf(cmd, sizeof(cmd), "AT+READ=%s,%u,%u", mt_uhf_get_membank_name(bank), offset, len);
    if (ret < 0)
        return ret;

    if (mask) {
        size_t mask_len = min(mask->fill, 16);
        if (sizeof(cmd) < ret + 2 * mask_len + 2)
            return -ENOBUFS;
        //add epc buffer
        cmd[ret++] = ',';
        mt_print_hex(mask->data, mask_len, cmd + ret);
        ret += 2 * mask_len;
    }
    cmd[ret++] = '\0';

    struct rw_usr_data usr_data = {
        .answer_buffer = answer_buffer_data,
        .epc_buffer    = answer_buffer_epc,
        .size          = answer_buffer_count,
        .fill          = 0,
        .error         = 0,
    };
    ret = mt_uhf_get_data(cmd, _read_data_cb, &usr_data, 0);
    if (ret)
        return ret;
    if (error_count)
        *error_count = usr_data.error;
    return usr_data.fill;
}

static int _write_data_cb(const char *prefix, char *data, bool finalized)
{
    mt_rfid_reader_assert(data && prefix, "Missing data or prefix");
    mt_rfid_reader_assert(!strcmp(prefix_WRT, prefix), "Wrong prefix");
    struct rw_usr_data *ud = reader.cmd.usr_data;
    mt_rfid_reader_assert(ud && !ud->answer_buffer,
                          "No user data or unexpected answer buffer in write");
    mt_rfid_reader_assert(ud && (!ud->epc_buffer == !ud->size), "Size and pointer mismatch");

    if (!data)
        return -EBADMSG;
    if (!strcmp(data, "<NO TAGS FOUND>")) {
        mt_rfid_reader_assert(ud->fill == 0, "No tags found so fill should be zero");
        mt_rfid_reader_assert(ud->error == 0, "No tags found so error should be zero");
        return EXIT_SUCCESS;
    }

    char *epc   = data;
    char *colon = strchr(epc, ',');
    if (!colon)
        return -EBADMSG;
    size_t epc_len = colon - epc;
    char  *result  = colon + 1;

    if (epc_len % 4 || epc_len > MT_UHF_GEN2_MAX_EPC_BYTES * 2)
        return -EBADMSG;
    if (!mt_parse_check_hex(epc, epc_len))
        return -EBADMSG;
    enum mt_uhf_rw_error_id result_id = mt_uhf_rw_get_error_id_from_name(result);
    if (result_id >= mt_uhf_rw_error_type_length)
        return -EBADMSG;
    //everything is parsed or parsable, no data errors

    if (result_id != mt_uhf_rw_error_type_no_error) {
        ud->error++;
        return EXIT_SUCCESS;
    }
    if (ud->fill < ud->size && ud->epc_buffer) {
        struct mt_uhf_buffer_epc *e = ud->epc_buffer + ud->fill;
        e->fill                     = epc_len / 2;
        mt_parse_hex_array_to_bytes(epc, epc_len, false, e->data, e->fill);
    }
    ud->fill++;
    return EXIT_SUCCESS;
}

int mt_uhf_write_data(enum mt_uhf_mem_bank      bank,
                      uint8_t                  *data,
                      unsigned                  data_len,
                      uint32_t                  offset,
                      struct mt_uhf_buffer_epc *answer_buffer_epc,
                      unsigned                  answer_buffer_count,
                      struct mt_uhf_buffer_epc *mask,
                      unsigned                 *error_count)
{
    if (!data || data_len == 0 || data_len > 16 || data_len & 1 || offset & 1)
        return -EINVAL;
    if (bank >= mt_uhf_mem_bank_length || bank == mt_uhf_mem_bank_OFF)
        return -EINVAL;
    if (answer_buffer_count && !answer_buffer_epc)
        return -EINVAL;

    char cmd[32 + 32 + 32]; //32 for command, 32 for data, 64 for mask
    int  ret = snprintf(cmd, sizeof(cmd), "AT+WRT=%s,%u,", mt_uhf_get_membank_name(bank), offset);
    if (ret < 0)
        return ret;
    //Add data
    if (ret + 2 * data_len + 1 > sizeof(cmd))
        return -ENOBUFS;
    mt_print_hex(data, data_len, cmd + ret);
    ret += 2 * data_len;

    //Add mask if provided
    if (mask && mask->fill) {
        size_t mask_len = min(mask->fill, 16);
        if (sizeof(cmd) < ret + 2 * mask_len + 2)
            return -ENOBUFS;
        cmd[ret++] = ',';
        mt_print_hex(mask->data, mask_len, cmd + ret);
        ret += 2 * mask_len;
    }
    cmd[ret++] = '\0';

    struct rw_usr_data usr_data = {
        .answer_buffer = NULL,
        .epc_buffer    = answer_buffer_epc,
        .size          = answer_buffer_count,
        .fill          = 0,
        .error         = 0,
    };
    ret = mt_uhf_get_data(cmd, _write_data_cb, &usr_data, 0);
    if (ret)
        return ret;
    if (error_count)
        *error_count = usr_data.error;
    return usr_data.fill;
}
