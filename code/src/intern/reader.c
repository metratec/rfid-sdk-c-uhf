/**
 * @file: reader.c                                                                                 *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-28                                                                        *
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

struct mt_uhf_allowed_mode_region_pair {
    enum mt_uhf_rf_mode mode;
    enum mt_uhf_region  regions;
};
const struct mt_uhf_allowed_mode_region_pair mt_uhf_allowed_mode_region_pairs[] =
    MT_DEVICE_MODE_ALLOWED_FOR_REGION;

const char *selected_strings[mt_uhf_gen2_inventory_selecting_length] = { "NSL", "SEL", "ALL" };

static const char *region_names[] = { "ETSI", "ETSI_HIGH", "FCC", "CHINA" };
const char        *mt_uhf_get_region_name(enum mt_uhf_region region)
{
    for (int i = 0; i < ARRAY_SIZE(region_names); i++)
        if ((1 << i) == region)
            return region_names[i];
    return NULL;
}

static const char *bank_names[mt_uhf_mem_bank_length] = { "EPC", "PC", "TID", "USR", "OFF" };
const char        *mt_uhf_get_membank_name(enum mt_uhf_mem_bank mem_bank)
{
    return bank_names[mem_bank];
}
enum mt_uhf_mem_bank mt_uhf_get_membank(const char *name)
{
    for (int i = 0; i < mt_uhf_mem_bank_length; i++)
        if (0 == strcmp(bank_names[i], name))
            return (enum mt_uhf_mem_bank)i;
    return mt_uhf_mem_bank_none;
}

struct uhfv2_reader reader = {
    .state.session        = mt_uhf_session_unknown,
    .resp.default_timeout = { .base = 1000, .cinv_running = 10000 },
};

static int _frame_resolve(void)
{
    static char parser_buf[MT_UHF_PARSER_BUF_SIZE];
    if (reader.resp.frame.current.type != uhfv2_frametype_none)
        return -ENOBUFS; //can't receive a new frame if last one is unhandled

    int ret = at_rx_ring_getframe(parser_buf, sizeof(parser_buf), false);
    if (ret <= 0)
        return ret;

    ret = mt_uhf_framehandler(parser_buf, ret);
    if (ret == -EALREADY || ret == -EAGAIN)
        return -EAGAIN;
    if (ret != EXIT_SUCCESS)
        return ret;

    switch (reader.resp.state) {
    case uhf_resp_state_start: //waiting for start
        if (reader.resp.frame.current.type == uhfv2_frametype_answer_start) {
            if (reader.state.echo_enabled == mt_uhf_boolx_false)
                reader.resp.state = uhf_resp_state_end; //this includes accepting event data
            else                                        //this includes unknown
                reader.resp.state = uhf_resp_state_echo;
            ret = -EAGAIN;
        } else
            ret = -EBADMSG;
        break;
    case uhf_resp_state_echo: //waiting for echo
        if (reader.resp.frame.current.type == uhfv2_frametype_echo) {
            if (reader.state.echo_enabled == mt_uhf_boolx_none)
                reader.state.echo_enabled = mt_uhf_boolx_true;
            reader.resp.state = uhf_resp_state_end;
            ret               = -EAGAIN;
            break;
        } else if (reader.state.echo_enabled == mt_uhf_boolx_false) {
            ret                       = -EBADMSG;
            reader.state.echo_enabled = mt_uhf_boolx_none;
            break;
        }
        //else fall through, the data could be end already
    case uhf_resp_state_end: //waiting for end of answer, this includes accepting event data
        if (reader.resp.frame.current.type == uhfv2_frametype_answer_finish_success) {
            if (reader.resp.state == uhf_resp_state_echo)
                reader.state.echo_enabled = mt_uhf_boolx_false;
            ret = EXIT_SUCCESS;
        } else if (reader.resp.frame.current.type == uhfv2_frametype_answer_finish_failed) {
            if (reader.resp.state == uhf_resp_state_echo)
                reader.state.echo_enabled = mt_uhf_boolx_false;
            ret = -EFAULT;
        } else
            ret = -EBADMSG;
        break;
    }

    mt_uhf_current_frame_handled();
    if (ret == EXIT_SUCCESS || ret == -EFAULT)
        reader.resp.state = uhf_resp_state_start;
    return ret;
}

int mt_uhf_response_resolve(const bool block)
{
    mt_rfid_reader_assert(reader.cmd.timeout && reader.cmd.data, "No command started");
    int  ret      = -ENODATA;
    bool new_data = true; //or at least: May be

    while (1) {
        if (reader.rx.poll_cb) {
            ret = reader.rx.poll_cb();
            if (ret < 0) //error, just stop and return
                return ret;
            if (ret > 0)
                new_data = true;
            else
                ret = -ENODATA;
        } else
            new_data = mt_rfid_reader_rx_new_data();
        if (new_data) {
            do {
                ret = _frame_resolve(); //loop frames, it can be multiples
            } while (ret == -EAGAIN);
            if (ret == -ENODATA)
                new_data = false;
        }
        if (ret != -ENODATA)
            return ret;
        uint32_t now = mt_rfid_reader_get_time();
        if ((now - reader.cmd.start_time) >= reader.cmd.timeout) {
#if DEBUG_PRINTOUT_PRINTF
            printf("Timeout @%u to command %s\n",
                   mt_rfid_reader_get_time(),
                   reader.cmd.data ? reader.cmd.data : "None");
#endif
            return -ETIMEDOUT;
        }
        if (!block)
            return -EAGAIN;
        if (reader.cmd.blocking_cb)
            reader.cmd.blocking_cb();
        else if (!new_data)
            mt_cmd_wait(10);
    }
}

int mt_uhf_resolve(void)
{
    if (reader.cmd.timeout && reader.cmd.data)
        return mt_uhf_response_resolve(false);
    int ret = -EAGAIN;

    while (ret == -EAGAIN) {
        if (reader.cmd.blocking_cb)
            reader.cmd.blocking_cb();
        else
            mt_cmd_wait(10);
        if (reader.rx.poll_cb) {
            ret = reader.rx.poll_cb();
            if (ret < 0) //error, just stop and return
                break;
        }
        ret = _frame_resolve();
    }
    return ret;
}

int mt_uhf_setter_call(const char *cmd, uint32_t timeout)
{
    return mt_uhf_get_data(cmd, NULL, NULL, timeout);
}

int mt_uhf_device_reset(void)
{
    int ret               = mt_uhf_setter_call("AT+RST", 0);
    reader.id.known_parts = 0;

    reader.state.echo_enabled = mt_uhf_boolx_none;
    reader.state.rf_mode      = mt_uhf_rf_mode_unknown;
    reader.state.region       = mt_uhf_region_none_unknown;
    reader.state.session      = mt_uhf_session_unknown;
    reader.state.inv_format   = uhfv2_inventory_tag_data_format_unknown;

    reader.cmd.data = NULL;

    reader.resp.state                        = uhf_resp_state_start;
    reader.resp.frame.last_data_match        = NULL;
    reader.resp.frame.current.type           = uhfv2_frametype_none;
    reader.resp.frame.current.data           = NULL;
    reader.resp.frame.last.type              = uhfv2_frametype_none;
    reader.resp.frame.last.data              = NULL;
    reader.resp.running_cinv                 = false;
    reader.resp.default_timeout.cinv_running = 10000;
    reader.resp.default_timeout.base         = 1000;
    uint32_t start                           = mt_rfid_reader_get_time();

    while (1) {
        uint32_t done = mt_rfid_reader_get_time() - start;
        if (done >= 2000)
            break;
        mt_cmd_wait(2000 - done);
    }
    return ret;
}

void mt_uhf_set_polling(mt_uhf_poll_cb cb)
{
    reader.rx.poll_cb = cb;
}

static int _id_update_cb(const char *prefix, char *data, bool finalized)
{
    mt_rfid_reader_assert(prefix && data, "No prefix or data for id_update_cb");
    if (0 == strcmp(prefix, "+SW")) {
        if (reader.id.known_parts & 0x03)
            return -EBADMSG;
        size_t len      = strlen(data);
        size_t rev_len  = sizeof(reader.id.fw_rev) - 1;
        size_t name_len = len - rev_len - 1;
        if (name_len >= sizeof(reader.id.fw_name) || data[name_len] != ' ')
            return -EBADMSG;

        memcpy(reader.id.fw_name, data, name_len);
        //memset(reader.id.fw_name + name_len, 0, sizeof(reader.id.fw_name) - name_len);
        memcpy(reader.id.fw_rev, data + name_len + 1, rev_len);
        reader.id.known_parts |= 0x03;
        return EXIT_SUCCESS;
    }
    if (0 == strcmp(prefix, "+HW")) {
        if (reader.id.known_parts & 0x0C)
            return -EBADMSG;
        size_t len      = strlen(data);
        size_t rev_len  = sizeof(reader.id.hw_rev) - 1;
        size_t name_len = len - rev_len - 1;
        if (name_len >= sizeof(reader.id.hw_name) || data[name_len] != ' ')
            return -EBADMSG;

        memcpy(reader.id.hw_name, data, name_len);
        //memset(reader.id.hw_name + name_len, 0, sizeof(reader.id.hw_name) - name_len);
        memcpy(reader.id.hw_rev, data + name_len + 1, rev_len);
        reader.id.known_parts |= 0x0C;
        return EXIT_SUCCESS;
    }
    if (0 == strcmp(prefix, "+SERIAL")) {
        if (reader.id.known_parts & 0x10)
            return -EBADMSG;
        if (strlen(data) != sizeof(reader.id.serial) - 1)
            return -EBADMSG;
        memcpy(reader.id.serial, data, sizeof(reader.id.serial) - 1);
        reader.id.known_parts |= 0x10;
        return EXIT_SUCCESS;
    }
    return -EBADMSG;
}

int mt_update_reader_identification(void)
{
    reader.id.known_parts  = 0;
    reader.unknown_data_cb = _id_update_cb;
    int ret                = mt_uhf_get_data("ATI", NULL, NULL, 0);
    if (ret == EXIT_SUCCESS) {
        if (reader.id.known_parts != 0x1F)
            ret = -EBADMSG;
#ifdef MT_UHF_MINIMUM_FW
        /*Check if Minimum Firmware Version was returned*/
        int fw_rev;
        mt_parse_int(reader.id.fw_rev, sizeof(reader.id.fw_rev) - 1, &fw_rev);
        if (fw_rev < MT_UHF_MINIMUM_FW)
            ret = -EINVAL;
#endif
    }

    return ret;
}

static int _update_rf_mode_cb(const char *prefix, char *data, bool finalized)
{
    if (strcmp(prefix, "+RFM") || !data)
        return -EBADMSG;
    size_t data_len = strlen(data);
    int    value;
    int    ret = mt_parse_int(data, data_len, &value);
    if (ret)
        return ret;
    bool valid           = mt_uhf_rf_mode_valid((enum mt_uhf_rf_mode)value);
    reader.state.rf_mode = valid ? (enum mt_uhf_rf_mode)value : mt_uhf_rf_mode_unknown;
    return valid ? EXIT_SUCCESS : -EBADMSG;
}
int mt_uhf_update_rf_mode(void)
{
    reader.state.rf_mode = mt_uhf_rf_mode_unknown;
    int ret              = mt_uhf_get_data("AT+RFM?", _update_rf_mode_cb, NULL, 0);
    if (ret == EXIT_SUCCESS && (reader.state.rf_mode == mt_uhf_rf_mode_unknown))
        return -EBADMSG;
    return ret;
}

bool mt_uhf_rf_mode_valid(enum mt_uhf_rf_mode mode)
{
    switch (mode) {
    MT_DEVICE_VALID_MODES:
        return true;
    default:
        return false;
    }
}

bool mt_uhf_mode_matches_region(enum mt_uhf_region region, enum mt_uhf_rf_mode rf_mode)
{
    for (int i = 0; i < ARRAY_SIZE(mt_uhf_allowed_mode_region_pairs); i++) {
        if (mt_uhf_allowed_mode_region_pairs[i].mode != rf_mode)
            continue;
        //found the mode, return true if the requested region(s) is / are all supporting the mode
        return (mt_uhf_allowed_mode_region_pairs[i].regions & region) == region;
    }
    //Mode not found in list
    return false;
}

static int _update_region_cb(const char *prefix, char *data, bool finalized)
{
    if (strcmp(prefix, "+REG"))
        return -EBADMSG;
    for (int i = 0; i < ARRAY_SIZE(region_names); i++)
        if (!strcmp(data, region_names[i])) {
            reader.state.region = (1 << i);
            return EXIT_SUCCESS;
        }
    return -EBADMSG;
}
int mt_uhf_update_region(void)
{
    reader.state.region = mt_uhf_region_none_unknown;
    int ret             = mt_uhf_get_data("AT+REG?", _update_region_cb, NULL, 0);
    if (ret == EXIT_SUCCESS && (reader.state.region == mt_uhf_region_none_unknown))
        return -EBADMSG;
    return ret;
}

static int _update_session_cb(const char *prefix, char *data, bool finalized)
{
    if (strcmp(prefix, "+SES"))
        return -EBADMSG;
    size_t data_len = strlen(data);
    if (data_len == 4 && !memcmp(data, "AUTO", 4))
        reader.state.session = mt_uhf_session_auto_sl;
    else if (data_len == 1) {
        uint8_t v = data[0] - '0';
        if (v > mt_uhf_session_max)
            return -EBADMSG;
        reader.state.session = v;
    } else
        return -EBADMSG;
    return EXIT_SUCCESS;
}
int mt_uhf_update_session(void)
{
    reader.state.session = mt_uhf_session_unknown;
    int ret              = mt_uhf_get_data("AT+SES?", _update_session_cb, NULL, 0);
    if (ret == EXIT_SUCCESS && (reader.state.session == mt_uhf_session_unknown))
        return -EBADMSG;
    return ret;
}

void *mt_uhf_get_usr_data(void)
{
    return reader.cmd.usr_data;
}

static int _get_byte_array_cb(const char *prefix, char *data, bool finalized)
{
    if (!finalized || !reader.cmd.usr_data)
        return -EBADMSG;
    struct mt_uhf_byte_array *target = reader.cmd.usr_data;

    size_t      max_len  = target->data_len;
    const char *position = data;
    target->data_len     = 0;
    for (size_t v = 0;; position++) {
        uint8_t digit = *position - '0';
        if (digit <= 9) { //it's a number
            v = v * 10 + digit;
            continue;
        }
        if (v == 0) //something else, if there's no number until now its invalid
            return -EBADMSG;
        if (*position == '\0') { //it was the last one
            target->data[target->data_len++] = v;
            return EXIT_SUCCESS;
        }
        if (*position == ',') {
            target->data[target->data_len++] = v;
            if (target->data_len >= max_len)
                return -ENOBUFS;
            v = 0;
            continue;
        }
        return -EBADMSG;
    }
}

int mt_uhf_get_data(const char *cmd, mt_uhf_data_cb_t data_cb, void *usr_data, uint32_t timeout)
{
    static char rx_prefix[16];
    if (cmd[0] != 'A' || cmd[1] != 'T')
        return -EINVAL;
    const char *prefix_end_eq = strchr(cmd + 2, '=');
    const char *prefix_end_qm = strchr(cmd + 2, '?');
    if (prefix_end_eq && prefix_end_qm)
        return -EINVAL;
    const char *prefix_end = prefix_end_eq ? prefix_end_eq : prefix_end_qm;
    size_t      len        = prefix_end ? prefix_end - (cmd + 2) : strlen(cmd + 2);
    if (len < sizeof(rx_prefix)) {
        memcpy(rx_prefix, cmd + 2, len);
        rx_prefix[len]         = '\0';
        reader.cmd.rsp_data.id = rx_prefix;
        reader.cmd.rsp_data.cb = data_cb;
    } else
        reader.unknown_data_cb = data_cb;

    reader.cmd.start_time = mt_rfid_reader_get_time();
    if (timeout)
        reader.cmd.timeout = timeout;
    else
        reader.cmd.timeout = (reader.resp.running_cinv) ? reader.resp.default_timeout.cinv_running :
                                                          reader.resp.default_timeout.base;
    reader.cmd.data     = cmd;
    reader.cmd.usr_data = usr_data;

#if DEBUG_PRINTOUT_PRINTF
    printf("CMD @%u: %s\n", mt_rfid_reader_get_time(), cmd);
#endif
    mt_rfid_reader_tx((const uint8_t *)cmd, strlen(cmd));
    mt_rfid_reader_tx("\r", 1);

    int ret = mt_uhf_response_resolve(true);

    reader.cmd.data        = NULL;
    reader.cmd.usr_data    = NULL;
    reader.cmd.rsp_data.id = NULL;
    reader.unknown_data_cb = NULL;
    return ret;
}

int mt_uhf_get_data_byte_array(const char               *name,
                               struct mt_uhf_byte_array *usr_data,
                               uint32_t                  timeout)
{
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "AT+%s?", name);
    return mt_uhf_get_data(cmd, _get_byte_array_cb, usr_data, timeout);
}

int mt_uhf_get_inventory_setting_cb(const char *prefix, char *data, bool finalized)
{
    if (!finalized || !reader.cmd.usr_data || strcmp(prefix, "+INVS"))
        return -EBADMSG;
    struct mt_uhf_invs_usr_data *target = reader.cmd.usr_data;

    const char *position = data;
    //get ONT
    uint8_t pos_v = position[0] - '0';
    if (position[1] != ',' || (pos_v > 1))
        return -EBADMSG;
    target->only_new_tags = pos_v;
    position += 2;
    //get RSSI
    pos_v = position[0] - '0';
    if (position[1] != ',' || (pos_v > 1))
        return -EBADMSG;
    target->show_rssi = pos_v;
    position += 2;
    //get TID
    pos_v = position[0] - '0';
    if (position[1] != ',' || (pos_v > 1))
        return -EBADMSG;
    target->read_tid = pos_v;
    position += 2;
    //get FAST_START
    pos_v = position[0] - '0';
    if (position[1] != ',' || (pos_v > 1))
        return -EBADMSG;
    target->fast_start = pos_v;
    position += 2;
    //get PHASE
    pos_v = position[0] - '0';
    if (position[1] != ',' || (pos_v > 1))
        return -EBADMSG;
    target->show_phase = pos_v;
    position += 2;
    //get SELECTED
    {
        char *comma = strchr(position, ',');
        if (!comma)
            return -EBADMSG;
        size_t s_len = comma - position;
        for (int i = 0; i < mt_uhf_gen2_inventory_selecting_length; i++) {
            size_t len = strlen(selected_strings[i]);
            if (len == s_len && !memcmp(position, selected_strings[i], len)) {
                target->selected = i;
                position         = comma + 1;
                break;
            }
        }
        if (position < comma) //position is still before comma so nothing was fitting
            return -EBADMSG;
    }
    //get TARGET
    pos_v = position[0] - 'A';
    if ((position[1] != ',' && (position[1] != 0)) || (pos_v > 1))
        return -EBADMSG;
    target->target = pos_v + mt_uhf_gen2_inventory_target_A;
    position += 2;
#ifndef MT_UHF_RSSI_THRESHOLD_UNSUPPORTED
    //get RSSI_THRESHOLD
    int ret = mt_parse_int(position, strlen(position), &target->rssi_threshold);
    if (ret)
        return -EBADMSG;
#endif
    //finished
    return EXIT_SUCCESS;
}

int mt_uhf_get_inventory_mask_cb(const char *prefix, char *data, bool finalized)
{
    if (!finalized || !reader.cmd.usr_data || strcmp(prefix, "+BMSK"))
        return -EBADMSG;

    uint32_t             mask_start_bit = 0, mask_len_bit = 0;
    enum mt_uhf_mem_bank bank;

    char *colon = strchr(data, ',');
    if (!colon) {
        if (0 == strcmp(bank_names[mt_uhf_mem_bank_OFF], data)) {
            bank = mt_uhf_mem_bank_OFF;
            goto exit;
        }
        return -EBADMSG;
    }
    char  *bank_name     = data;
    size_t bank_name_len = colon - bank_name;

    for (bank = mt_uhf_mem_bank_first; bank < mt_uhf_mem_bank_length; bank++)
        if ((0 == memcmp(bank_names[bank], bank_name, bank_name_len)) &&
            bank_names[bank][bank_name_len] == '\0')
            break;
    if (bank >= mt_uhf_mem_bank_none || bank == mt_uhf_mem_bank_OFF)
        return -EBADMSG;

    //tokenize
    char *str_start_bit = colon + 1;
    colon               = strchr(str_start_bit, ',');
    if (!colon)
        return -EBADMSG;
    size_t str_start_bit_len = colon - str_start_bit;
    char  *str_mask          = colon + 1;
    colon                    = strchr(str_mask, ',');
    if (!colon)
        return -EBADMSG;
    size_t str_mask_len    = colon - str_mask;
    char  *str_bit_len     = colon + 1;
    size_t str_bit_len_len = strlen(str_bit_len);

    int parse;
    int ret = mt_parse_int(str_start_bit, str_start_bit_len, &parse);
    if (ret || parse < 0)
        return ret;
    mask_start_bit = parse;

    ret = mt_parse_int(str_bit_len, str_bit_len_len, &parse);
    if (ret || parse < 0)
        return ret;
    mask_len_bit = parse;

    if ((mask_len_bit + 3) / 4 != str_mask_len)
        return -EBADMSG;
    for (unsigned i = 0; i < str_mask_len; i++) {
        data[i] = str_mask[i] - '0';
        if (data[i] > 9) {
            data[i] = str_mask[i] - 'A';
            if (data[i] > 0xf - 0xa)
                return -EBADMSG;
            data[i] += 0x0A;
        }
    }
    data[str_mask_len] = 0; //to fill up to a full byte if it's an odd number of nibbles
    goto exit;

exit:
    struct mt_uhf_bmsk_usr_data *target = reader.cmd.usr_data;
    if (target->mask && target->mask_buffer_size * 8 < mask_len_bit)
        return -ENOBUFS;
    if (target->target)
        *target->target = bank;
    if (target->mask_start_bit)
        *target->mask_start_bit = mask_start_bit;
    if (target->mask_len_bit)
        *target->mask_len_bit = mask_len_bit;
    ;
    uint8_t *t = target->mask;
    uint8_t *s = (uint8_t *)data;
    for (unsigned to_parse = mask_len_bit;; to_parse -= 8) {
        uint8_t v = 16 * *s++;
        v += *s++;
        if (to_parse > 8)
            *t++ = v;
        else {
            *t = v & (0xFF << (8 - to_parse));
            break;
        }
    }
    return EXIT_SUCCESS;
}
#if MT_UHF_GPI_COUNT || MT_UHF_GPO_COUNT
struct get_io_usr_data {
    mt_uhf_boolx_t *buffer;
};
#endif

#if MT_UHF_GPI_COUNT
DEFINE_PREFIX(IN);

int mt_uhf_get_input_cb(const char *prefix, char *data, bool finalized)
{
    mt_rfid_reader_assert(data && prefix, "Missing data or prefix");
    mt_rfid_reader_assert(!strcmp(prefix_IN, prefix), "Wrong prefix");
    mt_uhf_boolx_t *buffer = reader.cmd.usr_data;
    mt_rfid_reader_assert(buffer, "No data");

    if (!data)
        return -EBADMSG;
    char *colon = strchr(data, ',');
    if (!colon)
        return -EBADMSG;
    char  *io        = data;
    size_t io_len    = colon - io;
    char  *value     = colon + 1;
    size_t value_len = strlen(value);

    int io_value;
    int ret = mt_parse_int(io, io_len, &io_value);
    if (ret)
        return ret;
    io_value -= 1; //it starts at 1
    if (io_value >= MT_UHF_GPI_COUNT)
        return -ENOBUFS;
    if (buffer[io_value] != mt_uhf_boolx_none) //already filled
        return -EBADMSG;
    if (value_len == 3 && !memcmp(value, "LOW", 3))
        buffer[io_value] = mt_uhf_boolx_false;
    else if (value_len == 4 && !memcmp(value, "HIGH", 4))
        buffer[io_value] = mt_uhf_boolx_true;
    else
        buffer[io_value] = mt_uhf_boolx_invalid;
    return EXIT_SUCCESS;
}

int mt_uhf_get_input(mt_uhf_input_states_t buffer)
{
    memset(buffer, 0, sizeof(mt_uhf_input_states_t));
    int ret = mt_uhf_get_data("AT+IN?", mt_uhf_get_input_cb, buffer, 0);
    if (ret)
        return ret;
    return ret;
}
#endif

#if MT_UHF_GPO_COUNT
int mt_uhf_set_output(mt_uhf_output_states_t buffer)
{
    char cmd[16 + MT_UHF_GPO_COUNT * 3];
    int  ret = snprintf(cmd, sizeof(cmd), "AT+OUT=");
    if (ret < 0)
        return ret;
    char *pos = cmd + ret;
    for (unsigned i = 0; i < MT_UHF_GPO_COUNT; i++) {
        switch (buffer[i]) {
        case mt_uhf_boolx_false:
            *pos++ = '0';
            break;
        case mt_uhf_boolx_true:
            *pos++ = '1';
            break;
        case mt_uhf_boolx_none:
            break;
        case mt_uhf_boolx_invalid:
        default:
            return -EINVAL;
        }
        *pos++ = ',';
    }
    *(pos - 1) = '\0';
    return mt_uhf_setter_call(cmd, 0);
}
#endif

static int _get_echo_cb(const char *prefix, char *data, bool finalized)
{
    mt_rfid_reader_assert(!strcmp(prefix, "E") && data, "No data");
    if (strlen(data) != 1)
        return -EBADMSG;
    mt_uhf_boolx_t *target = reader.cmd.usr_data;
    mt_rfid_reader_assert(target && *target == mt_uhf_boolx_none, "Echo cb on already read value");

    if (data[0] == '0')
        *target = mt_uhf_boolx_false;
    else if (data[0] == '1')
        *target = mt_uhf_boolx_true;
    else
        *target = mt_uhf_boolx_invalid;
    return reader.state.echo_enabled < mt_uhf_boolx_invalid ? EXIT_SUCCESS : -EBADMSG;
}
int mt_uhf_update_echo(void)
{
    //dont directly update echo as this state change can mess with the answer parsing
    reader.state.echo_enabled = mt_uhf_boolx_none;
    mt_uhf_boolx_t echo       = mt_uhf_boolx_none;
    int            ret        = mt_uhf_get_data("ATE?", _get_echo_cb, &echo, 0);
    if (ret == EXIT_SUCCESS)
        reader.state.echo_enabled = echo;
    return ret;
}