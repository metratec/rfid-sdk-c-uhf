/**
 * @file: api.c                                                                                    *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-28                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */
#include <stdio.h>
#include <string.h>

#include <metratec/uhf_reader/intern/common.h>
#include <metratec/uhf_reader/public/api.h>

#define TIMEOUT_DEFAULT            1000
#define TIMEOUT_CONTINIOUS_DEFAULT 10000

mt_uhf_errorcode_t mt_uhf_heartbeat_set(uint8_t time, mt_uhf_data_cb_t cb)
{
    if (time > 60 || (time && !cb))
        return mt_uhf_errorcode_invalid_parameter;
    char cmd[16];
    int  retp = snprintf(cmd, sizeof(cmd), "AT+HBT=%u", time);
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_no_buffer;
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;

    mt_uhf_errorcode_t ret = mt_uhf_setter_call(cmd, 0);
    if (ret == mt_uhf_errorcode_success)
        ret = mt_uhf_framehandler_set_data_cb("+HBT", cb);
    return ret;
}

mt_uhf_errorcode_t mt_uhf_get_identification(struct mt_uhf_reader_identification *id)
{
    if (id == NULL)
        return mt_uhf_errorcode_invalid_parameter;
    if (reader.id.known_parts != mt_uhf_reader_identification_known_parts_all) {
        mt_uhf_errorcode_t ret = mt_update_reader_identification();
        if (ret < mt_uhf_errorcode_success)
            return ret;
    }
    if (reader.id.known_parts != mt_uhf_reader_identification_known_parts_all)
        return mt_uhf_errorcode_general_fault;

    *id = reader.id.data;
    return mt_uhf_errorcode_success;
}

mt_uhf_errorcode_t mt_uhf_data_cb_do_nothing(const char *prefix, char *data, bool finalized)
{
    return mt_uhf_errorcode_success;
}

mt_uhf_errorcode_t mt_uhf_init(mt_uhf_unexpected_frame_cb_t unexpected_frame_cb,
                               mt_uhf_poll_cb_t             polling_cb,
                               mt_uhf_blocking_cb_t         blocking_cb,
                               mt_uhf_errorcode_t (*reset_tried_cb)(void))
{
    static uint8_t     rx_buffer[MT_UHF_SETTING_RXBUFFER_SIZE];
    mt_uhf_errorcode_t ret = at_rx_ring_init(rx_buffer, sizeof(rx_buffer));
    if (ret < mt_uhf_errorcode_success)
        return ret;
    reader.state.initialized                 = mt_uhf_boolx_invalid; //ongoing
    reader.unexpected_frame_cb               = unexpected_frame_cb;
    reader.cmd.blocking_cb                   = blocking_cb;
    reader.resp.default_timeout.base         = TIMEOUT_DEFAULT;
    reader.resp.default_timeout.cinv_running = TIMEOUT_CONTINIOUS_DEFAULT;
    mt_uhf_set_polling(polling_cb);
    //register heartbeat callback doing nothing
    ret = mt_uhf_framehandler_set_data_cb("+HBT", mt_uhf_data_cb_do_nothing);
    if (ret < mt_uhf_errorcode_success)
        return ret;
    ret = mt_uhf_device_reset();
    if (reset_tried_cb) {
        mt_uhf_errorcode_t ret_cb = reset_tried_cb();
        if (ret_cb < mt_uhf_errorcode_success)
            return ret_cb;
    }

    if (ret) {
        //try again (unknown states like HBT etc should be gone after first reset cmd)
        if (polling_cb) {
            while (polling_cb() > 0)
                at_rx_ring_flush();
        } else
            at_rx_ring_flush();
        ret = mt_uhf_device_reset();
        if (reset_tried_cb) {
            mt_uhf_errorcode_t ret_cb = reset_tried_cb();
            if (ret_cb < mt_uhf_errorcode_success)
                return ret_cb;
        }
        if (ret)
            return ret;
    }
    mt_uhf_boolx_t *echo = mt_uhf_reader_echo_get();
    if (!echo || *echo != mt_uhf_boolx_false) {
        if ((ret = mt_uhf_reader_echo_set(false)))
            return ret;
    }
    //test
    if ((ret = mt_uhf_setter_call("AT", 0)))
        return ret;
    if ((ret = mt_update_reader_identification())) {
        reader.state.initialized = mt_uhf_boolx_false;
        return ret;
    }
    reader.state.initialized = mt_uhf_boolx_true;
    if ((ret = mt_uhf_get_rf_mode(NULL, NULL))) {
        reader.state.initialized = mt_uhf_boolx_false;
        return ret;
    }
    if ((ret = mt_uhf_get_session(NULL))) {
        reader.state.initialized = mt_uhf_boolx_false;
        return ret;
    }
    if ((ret = mt_uhf_get_inventory_settings(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL))) {
        reader.state.initialized = mt_uhf_boolx_false;
        return ret;
    }

    return ret;
}

void mt_uhf_timeout_set(uint32_t timeout, uint32_t timeout_cinv)
{
    reader.resp.default_timeout.base         = timeout ? timeout : TIMEOUT_DEFAULT;
    reader.resp.default_timeout.cinv_running = timeout_cinv ? timeout_cinv :
                                                              TIMEOUT_CONTINIOUS_DEFAULT;
}

void mt_uhf_rx_reset(void)
{
    reader.cmd.data = NULL;

    at_rx_ring_flush();
    (void)mt_uhf_current_frame_handled(); //last
    (void)mt_uhf_current_frame_handled(); //current

    reader.resp.state                 = uhf_resp_state_start;
    reader.resp.frame.last_data_match = NULL;
    reader.resp.frame.current.type    = uhfv2_frametype_none;
    reader.resp.frame.current.data    = NULL;
    reader.resp.frame.last.type       = uhfv2_frametype_none;
    reader.resp.frame.last.data       = NULL;
}

mt_uhf_errorcode_t mt_uhf_reader_echo_set(bool enable)
{
    char cmd[16];
    int  retp = snprintf(cmd, sizeof(cmd), "ATE=%c", enable ? '1' : '0');
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_no_buffer;
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;

    mt_uhf_errorcode_t ret = mt_uhf_setter_call(cmd, 0);
    if (ret >= mt_uhf_errorcode_success)
        reader.state.echo_enabled = enable ? mt_uhf_boolx_true : mt_uhf_boolx_false;
    else
        reader.state.echo_enabled = mt_uhf_boolx_none;
    return ret;
}

mt_uhf_boolx_t *mt_uhf_reader_echo_get(void)
{
    if (reader.state.echo_enabled >= mt_uhf_boolx_invalid)
        reader.state.echo_enabled = mt_uhf_boolx_none;
    if (reader.state.echo_enabled != mt_uhf_boolx_none)
        return &reader.state.echo_enabled;
    mt_uhf_errorcode_t ret = mt_uhf_update_echo();
    if (ret >= mt_uhf_errorcode_success && reader.state.echo_enabled > mt_uhf_boolx_none)
        return &reader.state.echo_enabled;
    return NULL;
}

mt_uhf_errorcode_t mt_uhf_set_q_value(uint8_t q_initial, uint8_t q_min, uint8_t q_max)
{
    if (q_initial < q_min || q_initial > q_max || q_max > 15) //q_min cant be below 0 anyways
        return mt_uhf_errorcode_invalid_parameter;
    //Build Cmd
    char cmd[16];
    int  retp = snprintf(cmd, sizeof(cmd), "AT+Q=%u,%u,%u", q_initial, q_min, q_max);
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_no_buffer;
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;

    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_set_power(const uint8_t *power, size_t num_of_antennas)
{
    if (!power || num_of_antennas > MAX_ANTENNAS_MUXED)
        return mt_uhf_errorcode_invalid_parameter;

    //Build Cmd
    //Handling possible power value up to 999, with a comma it's 4 chars per additional antenna
    char cmd[16 + MAX_ANTENNAS_MUXED * 4]; //up to 4 bytes per antenna
    if (num_of_antennas == 1 && MAX_ANTENNAS_MUXED == 1)
        num_of_antennas = 0; //use the "all" command instead
    if (num_of_antennas) {
        size_t fill = 0;
        for (int i = 0; i < num_of_antennas; i++) {
            if (power[i] < MT_UHF_POWER_MIN || power[i] > MT_UHF_POWER_MAX)
                return mt_uhf_errorcode_invalid_parameter;
            size_t space = sizeof(cmd) - fill;
            int    ret   = snprintf(cmd + fill, space, (i == 0) ? "AT+PWR=%u" : ",%u", power[i]);
            if (ret < 0)
                return mt_uhf_errorcode_general_fault;
            if (ret >= space)
                return mt_uhf_errorcode_invalid_parameter;
            fill += ret;
        }
    } else {
        if (power[0] < MT_UHF_POWER_MIN || power[0] > MT_UHF_POWER_MAX)
            return mt_uhf_errorcode_invalid_parameter;
        int ret = snprintf(cmd, sizeof(cmd), "AT+PWR=%u", power[0]);
        if (ret < 0)
            return mt_uhf_errorcode_general_fault;
        if (ret >= sizeof(cmd))
            return mt_uhf_errorcode_invalid_parameter;
    }

    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_get_power(uint8_t *power, size_t num_of_antennas)
{
    if (!power || num_of_antennas == 0)
        return mt_uhf_errorcode_invalid_parameter; //invalid parameters

    struct mt_uhf_byte_array answer = { .data = power, .data_len = num_of_antennas };
    mt_uhf_errorcode_t       ret    = mt_uhf_get_data_byte_array("PWR", &answer, 0);
    if (ret)
        return ret < 0 ? ret : mt_uhf_errorcode_inconsistent;
    if (answer.data_len == 0)
        return mt_uhf_errorcode_no_data_available;
    mt_rfid_reader_assert(answer.data_len <= MAX_ANTENNAS_MUXED,
                          "Reader sent more answers than expected by hardware");
    mt_rfid_reader_assert(answer.data_len <= num_of_antennas, "Invalid number of data");
    return answer.data_len;
}

int mt_uhf_set_rf_mode(enum mt_uhf_region region, enum mt_uhf_rf_mode rf_mode)
{
    //Check if all values are valid and match
    if (rf_mode == mt_uhf_rf_mode_unknown || region == mt_uhf_region_none_unknown)
        return mt_uhf_errorcode_invalid_parameter;
    if (!mt_uhf_rf_mode_valid(rf_mode))
        return mt_uhf_errorcode_invalid_parameter;
    if (!(region & mt_uhf_region_all))
        return mt_uhf_errorcode_invalid_parameter;
    if (!(region & MT_DEVICE_VALID_REGIONS))
        return mt_uhf_errorcode_not_supported;
    if (!mt_uhf_mode_matches_region(region, rf_mode))
        return mt_uhf_errorcode_not_supported;

    //then get current state to check if parts already fit
    char                cmd[32];
    enum mt_uhf_region  _region;
    enum mt_uhf_rf_mode _rf_mode;
    mt_uhf_errorcode_t  ret;
    if ((ret = mt_uhf_get_rf_mode(&_region, &_rf_mode)))
        return ret;
    if (_region == region)
        goto set_mode;
    const char *region_name = mt_uhf_get_region_name(region);
    if (!region_name)
        return mt_uhf_errorcode_invalid_parameter;
    int retp = snprintf(cmd, sizeof(cmd), "AT+REG=%s", region_name);
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_invalid_parameter;
    ret = mt_uhf_setter_call(cmd, 0);
    if (ret) {
        reader.state.region  = mt_uhf_region_none_unknown;
        reader.state.rf_mode = mt_uhf_rf_mode_unknown;
        return ret;
    }
    reader.state.region = region;
    //maybe a later implementation resets the mode on region change
    reader.state.rf_mode = mt_uhf_rf_mode_unknown;
    if ((ret = mt_uhf_get_rf_mode(NULL, &_rf_mode)))
        return ret;
    goto set_mode;

set_mode:
    if (_rf_mode == rf_mode)
        return mt_uhf_errorcode_success;

    retp = snprintf(cmd, sizeof(cmd), "AT+RFM=%u", rf_mode);
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_invalid_parameter;
    ret                  = mt_uhf_setter_call(cmd, 0);
    reader.state.rf_mode = (ret >= mt_uhf_errorcode_success) ? rf_mode : mt_uhf_rf_mode_unknown;
    return ret;
}

int mt_uhf_get_rf_mode(enum mt_uhf_region *region, enum mt_uhf_rf_mode *rf_mode)
{
    mt_uhf_errorcode_t ret = mt_uhf_errorcode_success;
    if (reader.state.rf_mode == mt_uhf_rf_mode_unknown)
        if ((ret = mt_uhf_update_rf_mode()))
            return ret;
    if (reader.state.region == mt_uhf_region_none_unknown)
        if ((ret = mt_uhf_update_region()))
            return ret;

    if (rf_mode)
        *rf_mode = reader.state.rf_mode;
    if (region)
        *region = reader.state.region;

    return ret;
}

int mt_uhf_get_session(enum mt_uhf_session *session)
{
    mt_uhf_errorcode_t ret = mt_uhf_errorcode_success;
    if (reader.state.session == mt_uhf_session_unknown)
        if ((ret = mt_uhf_update_session()))
            return ret;
    if (session)
        *session = reader.state.session;
    return ret;
}

int mt_uhf_set_session(enum mt_uhf_session session)
{
    //Build Cmd
    char cmd[16];
    int  retp = 0;
    switch (session) {
    case mt_uhf_session_s0:
    case mt_uhf_session_s1:
    case mt_uhf_session_s2:
    case mt_uhf_session_s3:
        retp = snprintf(cmd, sizeof(cmd), "AT+SES=%u", session);
        break;
    case mt_uhf_session_auto_sl:
        retp = snprintf(cmd, sizeof(cmd), "AT+SES=%s", "AUTO");
        break;
    default:
        return mt_uhf_errorcode_invalid_parameter;
    }
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_invalid_parameter;
    reader.state.session = mt_uhf_session_unknown;
    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_set_antenna(uint8_t antenna)
{
    if (antenna == 0 || antenna > MT_UHF_ANTENNA_COUNT)
        return mt_uhf_errorcode_invalid_parameter;
    char cmd[16];
    int  retp = snprintf(cmd, sizeof(cmd), "AT+ANT=%u", antenna);
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_invalid_parameter;
    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_set_multiplex_antennas(const uint8_t *mux_list, size_t mux_list_len)
{
    mt_rfid_reader_assert(mux_list_len <= MAX_ANTENNAS_MUXED, "Too many antennas for muxing");
    if (mux_list_len == 0 || mux_list_len > MAX_ANTENNAS_MUXED)
        return mt_uhf_errorcode_invalid_parameter;

    if (!mux_list) {
        char cmd[16];
        int  ret = snprintf(cmd, sizeof(cmd), "AT+MUX=%zu", mux_list_len);
        if (ret < 0)
            return ret;
        if (ret >= sizeof(cmd))
            return mt_uhf_errorcode_invalid_parameter; //too many data
        return mt_uhf_setter_call(cmd, 0);
    }
    for (size_t i = 0; i < mux_list_len; i++)
        if (mux_list[i] == 0 || mux_list[i] > MAX_ANTENNAS_MUXED)
            return mt_uhf_errorcode_invalid_parameter;

    //Handling possible antenna id up to 999, with a comma it's 4 chars per additional antenna
    mt_rfid_reader_assert(MAX_ANTENNAS_MUXED < 1000, "Unsafe memory");
    char   cmd[16 + MAX_ANTENNAS_MUXED * 4];
    size_t fill = 0;

    for (size_t i = 0; i < mux_list_len; i++) {
        size_t space = sizeof(cmd) - fill;
        int    ret   = snprintf(cmd + fill, space, i ? ",%u" : "AT+MUX=%u", mux_list[i]);
        if (ret < 0)
            return mt_uhf_errorcode_invalid_parameter;
        if (ret >= space)
            return mt_uhf_errorcode_invalid_parameter; //too long for internal buffer
        fill += ret;
    }
    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_get_multiplex_antennas(uint8_t *mux_list, size_t mux_list_len)
{
    struct mt_uhf_byte_array target = { mux_list, mux_list_len };
    mt_uhf_errorcode_t       ret    = mt_uhf_get_data_byte_array("MUX", &target, 0);
    if (ret)
        return ret < 0 ? ret : mt_uhf_errorcode_inconsistent;
    if (target.data_len == 0)
        return mt_uhf_errorcode_no_data_available;
    for (int i = 0; i < target.data_len; i++)
        if (target.data[i] == 0 || target.data[i] > MAX_ANTENNAS_MUXED)
            return mt_uhf_errorcode_range;
    return target.data_len;
}

int mt_uhf_set_external_multiplexer(uint8_t mux_list[MT_UHF_ANTENNA_COUNT])
{
#if MT_UHF_MUX_MAX == 0
    return mt_uhf_errorcode_not_supported;
#endif
    if (!mux_list)
        return mt_uhf_errorcode_invalid_parameter;

    //Handling possible mux id up to 999, with a comma it's 4 chars per additional antenna
    mt_rfid_reader_assert(MT_UHF_MUX_MAX < 1000, "Unsafe memory");
    char   cmd[16 + 4 * MT_UHF_ANTENNA_COUNT];
    size_t fill = 0;
    for (int i = 0; i < MT_UHF_ANTENNA_COUNT; i++) {
        if (mux_list[i] > MT_UHF_MUX_MAX)
            return mt_uhf_errorcode_invalid_parameter;
        size_t space = sizeof(cmd) - fill;
        int    ret   = snprintf(cmd + fill, space, (i == 0) ? "AT+EMX=%u" : ",%u", mux_list[i]);
        if (ret < 0)
            return ret;
        if (ret >= space)
            return mt_uhf_errorcode_invalid_parameter; //too long for buffer
        fill += ret;
    }
    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_get_external_multiplexer(uint8_t *mux_list)
{
    if (MT_UHF_ANTENNA_COUNT == 1)
        return mt_uhf_errorcode_not_supported;
    if (!mux_list)
        return mt_uhf_errorcode_invalid_parameter;

    struct mt_uhf_byte_array target = { mux_list, MT_UHF_ANTENNA_COUNT };
    mt_uhf_errorcode_t       ret    = mt_uhf_get_data_byte_array("EMX", &target, 0);
    if (ret)
        return ret < 0 ? ret : mt_uhf_errorcode_inconsistent;

    mt_rfid_reader_assert(target.data_len <= MT_UHF_ANTENNA_COUNT, "Invalid number of data");
    for (int i = 0; i < target.data_len; i++)
        if (target.data[i] > MT_UHF_MUX_MAX)
            return mt_uhf_errorcode_range;
    return target.data_len;
}

int mt_uhf_set_inventory_settings(bool                                 only_new_tags,
                                  bool                                 show_rssi,
                                  bool                                 read_tid,
                                  bool                                 fast_start,
                                  bool                                 show_phase,
                                  enum mt_uhf_gen2_inventory_selecting selected,
                                  enum mt_uhf_gen2_inventory_target    target,
                                  int                                  rssi_threshold)
{
#ifdef MT_UHF_RSSI_THRESHOLD_UNSUPPORTED
    (void)rssi_threshold;
#else
    if (rssi_threshold > -20 || rssi_threshold < -100)
        return mt_uhf_errorcode_invalid_parameter;
#endif
    if (selected >= mt_uhf_gen2_inventory_selecting_length ||
        target >= mt_uhf_gen2_inventory_target_length)
        return mt_uhf_errorcode_invalid_parameter;

    char cmd[64];
#ifdef MT_UHF_RSSI_THRESHOLD_UNSUPPORTED
    int retp = snprintf(cmd,
                        sizeof(cmd),
                        "AT+INVS=%c,%c,%c,%c,%c,%s,%c",
                        only_new_tags ? '1' : '0',
                        show_rssi ? '1' : '0',
                        read_tid ? '1' : '0',
                        fast_start ? '1' : '0',
                        show_phase ? '1' : '0',
                        selected_strings[selected],
                        target == mt_uhf_gen2_inventory_target_B ? 'B' : 'A');
#else
    int retp = snprintf(cmd,
                        sizeof(cmd),
                        "AT+INVS=%c,%c,%c,%c,%c,%s,%c,%i",
                        only_new_tags ? '1' : '0',
                        show_rssi ? '1' : '0',
                        read_tid ? '1' : '0',
                        fast_start ? '1' : '0',
                        show_phase ? '1' : '0',
                        selected_strings[selected],
                        target == mt_uhf_gen2_inventory_target_B ? 'B' : 'A',
                        rssi_threshold);
#endif
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_invalid_parameter;

    mt_uhf_errorcode_t ret = mt_uhf_setter_call(cmd, 0);
    if (ret >= mt_uhf_errorcode_success) {
        reader.state.only_new_tag = only_new_tags ? mt_uhf_boolx_true : mt_uhf_boolx_false;
        reader.state.fast_start   = fast_start ? mt_uhf_boolx_true : mt_uhf_boolx_false;
        reader.state.inv_target   = target;
        reader.state.inv_selected = selected;
        reader.state.inv_format   = uhfv2_inventory_tag_data_format_epc;
        if (show_rssi)
            reader.state.inv_format |= uhfv2_inventory_tag_data_format_RSSI;
        if (read_tid)
            reader.state.inv_format |= uhfv2_inventory_tag_data_format_TID;
        if (show_phase)
            reader.state.inv_format |= uhfv2_inventory_tag_data_format_Phase;
    } else
        reader.state.inv_format = uhfv2_inventory_tag_data_format_unknown;
    return ret;
}

/**
 * @brief               Callback function used to get the INVS answer
 * @param prefix 
 * @param data 
 * @param finalized 
 * 
 * @return              mt_uhf_errorcode_success (0) on success
 * @returns             < 0: Error code 
 */
static mt_uhf_errorcode_t _get_inventory_setting_cb(const char *prefix, char *data, bool finalized)
{
    if (!finalized || !reader.cmd.usr_data || strcmp(prefix, "+INVS"))
        return mt_uhf_errorcode_format_fault;
    struct mt_uhf_invs_usr_data *target = reader.cmd.usr_data;

    const char *position = data;
    //get ONT
    uint8_t pos_v = position[0] - '0';
    if (position[1] != ',' || (pos_v > 1))
        return mt_uhf_errorcode_format_fault;
    target->only_new_tags = pos_v;
    position += 2;
    //get RSSI
    pos_v = position[0] - '0';
    if (position[1] != ',' || (pos_v > 1))
        return mt_uhf_errorcode_format_fault;
    target->show_rssi = pos_v;
    position += 2;
    //get TID
    pos_v = position[0] - '0';
    if (position[1] != ',' || (pos_v > 1))
        return mt_uhf_errorcode_format_fault;
    target->read_tid = pos_v;
    position += 2;
    //get FAST_START
    pos_v = position[0] - '0';
    if (position[1] != ',' || (pos_v > 1))
        return mt_uhf_errorcode_format_fault;
    target->fast_start = pos_v;
    position += 2;
    //get PHASE
    pos_v = position[0] - '0';
    if (position[1] != ',' || (pos_v > 1))
        return mt_uhf_errorcode_format_fault;
    target->show_phase = pos_v;
    position += 2;
    //get SELECTED
    {
        char *comma = strchr(position, ',');
        if (!comma)
            return mt_uhf_errorcode_format_fault;
        size_t s_len = comma - position;
        if (s_len == 0)
            return mt_uhf_errorcode_format_fault;
        for (int i = 0; i < mt_uhf_gen2_inventory_selecting_length; i++) {
            size_t len = strlen(selected_strings[i]);
            if (len == s_len && !memcmp(position, selected_strings[i], len)) {
                target->selected = i;
                position         = comma + 1;
                break;
            }
        }
        if (position < comma) //position is still before comma so nothing was fitting
            return mt_uhf_errorcode_format_fault;
    }
    //get TARGET
    pos_v = position[0] - 'A';
    if ((position[1] != ',' && (position[1] != 0)) || (pos_v > 1))
        return mt_uhf_errorcode_format_fault;
    target->target = pos_v + mt_uhf_gen2_inventory_target_A;
    position += 2;
#ifndef MT_UHF_RSSI_THRESHOLD_UNSUPPORTED
    //get RSSI_THRESHOLD
    mt_uhf_errorcode_t ret = mt_parse_int(position, strlen(position), &target->rssi_threshold);
    if (ret < mt_uhf_errorcode_success)
        return mt_uhf_errorcode_format_fault;
#endif
    //finished
    return mt_uhf_errorcode_success;
}

mt_uhf_errorcode_t mt_uhf_get_inventory_settings(bool *only_new_tags,
                                                 bool *show_rssi,
                                                 bool *read_tid,
                                                 bool *fast_start,
                                                 bool *show_phase,
                                                 enum mt_uhf_gen2_inventory_selecting *selected,
                                                 enum mt_uhf_gen2_inventory_target    *target,
                                                 int *rssi_threshold)
{
#ifdef MT_UHF_RSSI_THRESHOLD_UNSUPPORTED
    if (rssi_threshold)
        return mt_uhf_errorcode_invalid_parameter;
#else
    (void)rssi_threshold;
#endif
#define COPY_ELEMENT_IF_NEEDED(element) \
    if (element)                        \
        *element = settings.element;

    struct mt_uhf_invs_usr_data settings;
    mt_uhf_errorcode_t ret = mt_uhf_get_data("AT+INVS?", _get_inventory_setting_cb, &settings, 0);

    if (ret >= mt_uhf_errorcode_success) {
        COPY_ELEMENT_IF_NEEDED(only_new_tags);
        COPY_ELEMENT_IF_NEEDED(show_rssi);
        COPY_ELEMENT_IF_NEEDED(read_tid);
        COPY_ELEMENT_IF_NEEDED(fast_start);
        COPY_ELEMENT_IF_NEEDED(show_phase);
        COPY_ELEMENT_IF_NEEDED(selected);
        COPY_ELEMENT_IF_NEEDED(target);
        COPY_ELEMENT_IF_NEEDED(rssi_threshold);
        reader.state.inv_format = uhfv2_inventory_tag_data_format_epc;
        if (settings.show_rssi)
            reader.state.inv_format |= uhfv2_inventory_tag_data_format_RSSI;
        if (settings.read_tid)
            reader.state.inv_format |= uhfv2_inventory_tag_data_format_TID;
        if (settings.show_phase)
            reader.state.inv_format |= uhfv2_inventory_tag_data_format_Phase;
    }
    return ret;
}

mt_uhf_errorcode_t mt_uhf_set_inventory_mask(enum mt_uhf_mem_bank target,
                                             uint32_t             mask_start_bit,
                                             const uint8_t       *mask,
                                             uint32_t             mask_len_bit)
{
    if (target >= mt_uhf_mem_bank_length)
        return mt_uhf_errorcode_invalid_parameter;
    if (target != mt_uhf_mem_bank_OFF && (mask == NULL || mask_len_bit == 0))
        return mt_uhf_errorcode_invalid_parameter;
    if (mask_len_bit && !mask)
        return mt_uhf_errorcode_invalid_parameter;
    char cmd[128];
    int  retp = snprintf(cmd, sizeof(cmd), "AT+BMSK=%s", mt_uhf_get_membank_name(target));
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_invalid_parameter;
    if (target != mt_uhf_mem_bank_OFF) {
        size_t offset = retp;
        retp          = snprintf(cmd + offset, sizeof(cmd) - offset, ",%u", mask_start_bit);
        if (retp < 0)
            return mt_uhf_errorcode_general_fault;
        offset += retp;
        if (offset + 2 >= sizeof(cmd))
            return mt_uhf_errorcode_invalid_parameter;
        cmd[offset++]         = ',';
        size_t mask_len_bytes = (mask_len_bit + 7) / 8;
        if (sizeof(cmd) - offset < 2 * mask_len_bytes + 3)
            return mt_uhf_errorcode_invalid_parameter;
        mt_print_hex(mask, mask_len_bytes, cmd + offset);
        offset += 2 * mask_len_bytes;
        retp = snprintf(cmd + offset, sizeof(cmd) - offset, ",%u", mask_len_bit);
        if (retp < 0)
            return mt_uhf_errorcode_general_fault;
        offset += retp;
        if (offset >= sizeof(cmd))
            return mt_uhf_errorcode_invalid_parameter;
    }
    return mt_uhf_setter_call(cmd, 0);
}

mt_uhf_errorcode_t mt_uhf_get_inventory_mask(enum mt_uhf_mem_bank *target,
                                             uint32_t             *mask_start_bit,
                                             uint8_t              *mask,
                                             size_t                mask_buffer_size,
                                             uint32_t             *mask_len_bit)
{
    struct mt_uhf_bmsk_usr_data usr_data = { .mask             = mask,
                                             .mask_len_bit     = mask_len_bit,
                                             .mask_start_bit   = mask_start_bit,
                                             .target           = target,
                                             .mask_buffer_size = mask_buffer_size };
    if (!mask ^ !mask_buffer_size)
        return mt_uhf_errorcode_invalid_parameter;
    return mt_uhf_get_data("AT+BMSK?", mt_uhf_get_inventory_mask_cb, &usr_data, 0);
}

mt_uhf_errorcode_t mt_uhf_set_channel_mask(uint64_t mask)
{
    if (reader.state.region != mt_uhf_region_etsi_lowerband)
        return mt_uhf_errorcode_not_available;
    if (mask > 0x0F) //there are only 4 channels in ETSI lower band
        return mt_uhf_errorcode_invalid_parameter;
    char cmd[128];
    int  retp = snprintf(cmd,
                        sizeof(cmd),
                        "AT+CMSK=%08X%08X",
                        (uint32_t)(mask >> 32),
                        (uint32_t)(mask & UINT32_MAX));
    if (retp < 0)
        return mt_uhf_errorcode_general_fault;
    if (retp >= sizeof(cmd))
        return mt_uhf_errorcode_invalid_parameter;
    return mt_uhf_setter_call(cmd, 0);
}

static mt_uhf_errorcode_t mt_uhf_get_channel_mask_cb(const char *prefix, char *data, bool finalized)
{
    if (!finalized || strcmp(prefix, "+CMSK") || !data)
        return mt_uhf_errorcode_format_fault;
    size_t  data_len = strlen(data);
    uint8_t buffer[8];
    if (data_len == 0 || data_len > 16 || data_len % 2)
        return mt_uhf_errorcode_range;
    mt_uhf_errorcode_t ret = mt_parse_hex_array_to_bytes(data, data_len, true, buffer, 8);
    if (ret < mt_uhf_errorcode_success)
        return mt_uhf_errorcode_range;
    if (reader.cmd.usr_data) {
        uint64_t *target = reader.cmd.usr_data;
        *target          = 0;
        for (int i = 0; i < data_len / 2; i++)
            *target = *target * 256 + buffer[i];
    }
    return mt_uhf_errorcode_success;
}

mt_uhf_errorcode_t mt_uhf_get_channel_mask(uint64_t *mask)
{
    if (!mask)
        return mt_uhf_errorcode_invalid_parameter;

    uint64_t           mask_buf;
    mt_uhf_errorcode_t ret = mt_uhf_get_data("AT+CMSK?", mt_uhf_get_channel_mask_cb, &mask_buf, 0);
    if (ret < mt_uhf_errorcode_success)
        return ret;

    uint64_t mask_all = (1ULL << 50) - 1;
    *mask             = mask_buf == mask_all ? 0 : mask_buf;
    return mt_uhf_errorcode_success;
}
