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

#include <metratec/uhf_reader_sdk.h>

//sdk needs to come first to include the settings
#include <metratec/uhf_reader/intern/reader.h>

#define TIMEOUT_DEFAULT            1000
#define TIMEOUT_CONTINIOUS_DEFAULT 10000

int mt_reader_heartbeat_set(uint8_t time, mt_uhf_data_cb_t cb)
{
    if (time > 60 || (time && !cb))
        return -EINVAL;
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "AT+HBT=%u", time);
    int ret = mt_uhf_setter_call(cmd, 0);
    if (ret == EXIT_SUCCESS)
        ret = mt_uhf_framehandler_set_data_cb("+HBT", cb);
    return ret;
}

struct mt_uhf_reader_identification *mt_uhf_get_identification(void)
{
    if (reader.id.known_parts != 0x1F)
        if (mt_update_reader_identification())
            return NULL;
    return (reader.id.known_parts == 0x1F) ? &reader.id : NULL;
}
int mt_uhf_data_cb_do_nothing(const char *prefix, char *data, bool finalized)
{
    return EXIT_SUCCESS;
}
int mt_uhf_init(int (*unknown_frame)(const char *),
                mt_uhf_poll_cb polling_cb,
                void (*blocking_cb)(void),
                int (*reset_done_cb)(void))
{
    static uint8_t rx_buffer[MT_UHF_SETTING_RXBUFFER_SIZE];
    int            ret = at_rx_ring_init(rx_buffer, sizeof(rx_buffer));
    if (ret != EXIT_SUCCESS)
        return ret;
    reader.unknown_frame_cb                  = unknown_frame;
    reader.cmd.blocking_cb                   = blocking_cb;
    reader.resp.default_timeout.base         = TIMEOUT_DEFAULT;
    reader.resp.default_timeout.cinv_running = TIMEOUT_CONTINIOUS_DEFAULT;
    mt_uhf_set_polling(polling_cb);
    mt_uhf_framehandler_set_data_cb("+HBT", mt_uhf_data_cb_do_nothing);
    ret = mt_uhf_device_reset();
    if (reset_done_cb)
        reset_done_cb();

    if (ret) { //try again (unknown states like HBT etc)
        if (polling_cb) {
            while (polling_cb() > 0)
                at_rx_ring_flush();
        } else
            at_rx_ring_flush();
        ret = mt_uhf_device_reset();
        reset_done_cb();
    }
    if (ret)
        return ret;
    mt_uhf_boolx_t *echo = mt_uhf_reader_echo_get();
    if (!echo || *echo != mt_uhf_boolx_false) {
        if ((ret = mt_uhf_reader_echo_set(false)))
            return ret;
    }
    //test
    if ((ret = mt_uhf_setter_call("AT", 0)))
        return ret;
    if ((ret = mt_update_reader_identification()))
        return ret;
    if ((ret = mt_uhf_get_rf_mode(NULL, NULL)))
        return ret;
    if ((ret = mt_uhf_get_session(NULL)))
        return ret;
    if ((ret = mt_uhf_get_inventory_settings(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL)))
        return ret;

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
    mt_uhf_current_frame_handled();
    mt_uhf_current_frame_handled();

    reader.resp.state                 = uhf_resp_state_start;
    reader.resp.frame.last_data_match = NULL;
    reader.resp.frame.current.type    = uhfv2_frametype_none;
    reader.resp.frame.current.data    = NULL;
    reader.resp.frame.last.type       = uhfv2_frametype_none;
    reader.resp.frame.last.data       = NULL;
}

int mt_uhf_reader_echo_set(bool enable)
{
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "ATE=%c", enable ? '1' : '0');
    int ret = mt_uhf_setter_call(cmd, 0);
    if (ret == EXIT_SUCCESS)
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
    int ret = mt_uhf_update_echo();
    if (ret == EXIT_SUCCESS && reader.state.echo_enabled > mt_uhf_boolx_none)
        return &reader.state.echo_enabled;
    return NULL;
}

int mt_uhf_set_q_value(uint8_t q_initial, uint8_t q_min, uint8_t q_max)
{
    if (q_initial < q_min || q_initial > q_max || q_max > 15) //q_min cant be below 0 anyways
        return -EINVAL;
    //Build Cmd
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "AT+Q=%u,%u,%u", q_initial, q_min, q_max);
    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_set_power(const uint8_t *power, size_t num_of_antennas)
{
    if (!power || num_of_antennas > MAX_ANTENNAS_MUXED)
        return -EINVAL; //invalid parameters

    //Build Cmd
    //Handling possible power value up to 999, with a comma it's 4 chars per additional antenna
    char cmd[16 + MAX_ANTENNAS_MUXED * 4]; //up to 4 bytes per antenna
    if (num_of_antennas == 1 && MAX_ANTENNAS_MUXED == 1)
        num_of_antennas = 0; //use the "all" command instead
    if (num_of_antennas) {
        size_t fill = 0;
        for (int i = 0; i < num_of_antennas; i++) {
            if (power[i] < MT_UHF_POWER_MIN || power[i] > MT_UHF_POWER_MAX)
                return -EINVAL;
            size_t space = sizeof(cmd) - fill;
            int    ret   = snprintf(cmd + fill, space, (i == 0) ? "AT+PWR=%u" : ",%u", power[i]);
            if (ret < 0)
                return ret;
            if (ret >= space)
                return -ENOBUFS;
            fill += ret;
        }
    } else {
        if (power[0] < MT_UHF_POWER_MIN || power[0] > MT_UHF_POWER_MAX)
            return -EINVAL;
        int ret = snprintf(cmd, sizeof(cmd), "AT+PWR=%u", power[0]);
        if (ret < 0)
            return ret;
        if (ret >= sizeof(cmd))
            return -ENOBUFS;
    }

    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_get_power(uint8_t *power, size_t num_of_antennas)
{
    if (!power || num_of_antennas == 0 || num_of_antennas > MAX_ANTENNAS_MUXED)
        return -EINVAL; //invalid parameters

    struct mt_uhf_byte_array answer = { .data = power, .data_len = num_of_antennas };
    int                      ret    = mt_uhf_get_data_byte_array("PWR", &answer, 0);
    if (ret)
        return ret;
    if (answer.data_len == 0 || answer.data_len > MAX_ANTENNAS_MUXED)
        return -EBADMSG;
    if (answer.data_len > num_of_antennas)
        return -EFAULT;
    return ret;
}

int mt_uhf_set_rf_mode(enum mt_uhf_region region, enum mt_uhf_rf_mode rf_mode)
{
    //Check if all values are valid and match
    if (rf_mode == mt_uhf_rf_mode_unknown || region == mt_uhf_region_none_unknown)
        return -EINVAL;
    if (!mt_uhf_rf_mode_valid(rf_mode))
        return -EINVAL;
    if (!(region & mt_uhf_region_all))
        return -EINVAL;
    if (!(region & MT_DEVICE_VALID_REGIONS))
        return -ENOTSUP;
    if (!mt_uhf_mode_matches_region(region, rf_mode))
        return -ENOTSUP;

    //then get current state to check if parts already fit
    char                cmd[32];
    enum mt_uhf_region  _region;
    enum mt_uhf_rf_mode _rf_mode;
    int                 ret;
    if ((ret = mt_uhf_get_rf_mode(&_region, &_rf_mode)))
        return ret;
    if (_region == region)
        goto set_mode;
    const char *region_name = mt_uhf_get_region_name(region);
    if (!region_name)
        return -EINVAL;
    snprintf(cmd, sizeof(cmd), "AT+REG=%s", region_name);
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
        return EXIT_SUCCESS;

    snprintf(cmd, sizeof(cmd), "AT+RFM=%u", rf_mode);
    ret                  = mt_uhf_setter_call(cmd, 0);
    reader.state.rf_mode = (ret == EXIT_SUCCESS) ? rf_mode : mt_uhf_rf_mode_unknown;
    return ret;
}

int mt_uhf_get_rf_mode(enum mt_uhf_region *region, enum mt_uhf_rf_mode *rf_mode)
{
    int ret = EXIT_SUCCESS;
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
    int ret = EXIT_SUCCESS;
    if (reader.state.session == mt_uhf_session_unknown)
        if ((ret = mt_uhf_update_session()))
            return ret;
    if (session)
        *session = reader.state.session;
    return ret;
}

int mt_uhf_set_session(enum mt_uhf_session session)
{
    reader.state.session = mt_uhf_session_unknown;
    //Build Cmd
    char cmd[16];
    switch (session) {
    case mt_uhf_session_s0:
    case mt_uhf_session_s1:
    case mt_uhf_session_s2:
    case mt_uhf_session_s3:
        snprintf(cmd, sizeof(cmd), "AT+SES=%u", session);
        break;
    case mt_uhf_session_auto_sl:
        snprintf(cmd, sizeof(cmd), "AT+SES=%s", "AUTO");
        break;
    default:
        return -EINVAL;
    }
    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_set_antenna(uint8_t antenna)
{
    if (antenna == 0 || antenna > MT_UHF_ANTENNA_COUNT)
        return -EINVAL; //Antenna id out of Range
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "AT+ANT=%u", antenna);
    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_set_multiplex_antennas(uint8_t *mux_list, size_t mux_list_len)
{
    mt_rfid_reader_assert(mux_list_len < MAX_ANTENNAS_MUXED, "Too many antennas for muxing");
    if (mux_list_len == 0 || mux_list_len > MAX_ANTENNAS_MUXED)
        return -EINVAL;
    if (!mux_list) {
        char cmd[16];
        int  ret = snprintf(cmd, sizeof(cmd), "AT+MUX=%zu", mux_list_len);
        if (ret < 0)
            return ret;
        if (ret >= sizeof(cmd))
            return -ENOBUFS;
        return mt_uhf_setter_call(cmd, 0);
    }
    for (size_t i = 0; i < mux_list_len; i++)
        if (mux_list[i] == 0 || mux_list[i] > MAX_ANTENNAS_MUXED)
            return -EINVAL;

    //Handling possible antenna id up to 999, with a comma it's 4 chars per additional antenna
    mt_rfid_reader_assert(MAX_ANTENNAS_MUXED >= 1000, "Unsafe memory");
    char   cmd[16 + MAX_ANTENNAS_MUXED * 4];
    size_t fill = 0;

    for (size_t i = 0; i < mux_list_len; i++) {
        size_t space = sizeof(cmd) - fill;
        int    ret   = snprintf(cmd + fill, space, i ? ",%u" : "AT+MUX=%u", mux_list[i]);
        if (ret < 0)
            return -EINVAL;
        if (ret >= space)
            return -ENOBUFS;
        fill += ret;
    }
    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_get_multiplex_antennas(uint8_t *mux_list, size_t mux_list_len)
{
    const size_t max_antennas       = MT_UHF_ANTENNA_COUNT * (MT_UHF_MUX_MAX ? MT_UHF_MUX_MAX : 1);
    struct mt_uhf_byte_array target = { mux_list, mux_list_len };
    int                      ret    = mt_uhf_get_data_byte_array("MUX", &target, 0);
    if (ret)
        return ret;
    if (target.data_len == 0)
        return -EBADMSG;
    for (int i = 0; i < target.data_len; i++)
        if (target.data[i] == 0 || target.data[i] > max_antennas)
            return -EBADMSG;
    return target.data_len;
}

int mt_uhf_set_external_multiplexer(uint8_t mux_list[MT_UHF_ANTENNA_COUNT])
{
#if MT_UHF_MUX_MAX == 0
    return -ENOTSUP;
#endif
    if (!mux_list)
        return -EINVAL;

    //Handling possible mux id up to 999, with a comma it's 4 chars per additional antenna
    mt_rfid_reader_assert(MT_UHF_MUX_MAX >= 1000, "Unsafe memory");
    char   cmd[16 + 4 * MT_UHF_ANTENNA_COUNT];
    size_t fill = 0;
    for (int i = 0; i < MT_UHF_ANTENNA_COUNT; i++) {
        if (mux_list[i] > MT_UHF_MUX_MAX)
            return -EINVAL;
        size_t space = sizeof(cmd) - fill;
        int    ret   = snprintf(cmd + fill, space, (i == 0) ? "AT+EMX=%u" : ",%u", mux_list[i]);
        if (ret < 0)
            return ret;
        if (ret >= space)
            return -ENOBUFS;
        fill += ret;
    }
    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_get_external_multiplexer(uint8_t *mux_list)
{
    if (MT_UHF_ANTENNA_COUNT == 1)
        return -ENOTSUP;
    if (!mux_list)
        return -EINVAL;

    struct mt_uhf_byte_array target = { mux_list, MT_UHF_ANTENNA_COUNT };
    int                      ret    = mt_uhf_get_data_byte_array("EMX", &target, 0);
    if (ret)
        return ret;
    if (target.data_len != MT_UHF_ANTENNA_COUNT)
        return -EBADMSG;
    for (int i = 0; i < target.data_len; i++)
        if (target.data[i] > MT_UHF_MUX_MAX)
            return -EBADMSG;
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
    if (rssi_threshold > -20 || rssi_threshold < -100 ||
        selected >= mt_uhf_gen2_inventory_selecting_length ||
        target >= mt_uhf_gen2_inventory_target_length)
        return -EINVAL;

    char cmd[64];
#ifdef MT_UHF_RSSI_THRESHOLD_UNSUPPORTED
    snprintf(cmd,
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
    snprintf(cmd,
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
    int ret = mt_uhf_setter_call(cmd, 0);
    if (ret == EXIT_SUCCESS) {
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

int mt_uhf_get_inventory_settings(bool                                 *only_new_tags,
                                  bool                                 *show_rssi,
                                  bool                                 *read_tid,
                                  bool                                 *fast_start,
                                  bool                                 *show_phase,
                                  enum mt_uhf_gen2_inventory_selecting *selected,
                                  enum mt_uhf_gen2_inventory_target    *target,
                                  int                                  *rssi_threshold)
{
#define COPY_ELEMENT_IF_NEEDED(element) \
    if (element)                        \
        *element = settings.element;

    struct mt_uhf_invs_usr_data settings;
    int ret = mt_uhf_get_data("AT+INVS?", &mt_uhf_get_inventory_setting_cb, &settings, 0);

    if (ret == EXIT_SUCCESS) {
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

int mt_uhf_set_inventory_mask(enum mt_uhf_mem_bank target,
                              uint32_t             mask_start_bit,
                              const uint8_t       *mask,
                              uint32_t             mask_len_bit)
{
    if (target >= mt_uhf_mem_bank_length)
        return -EINVAL;
    if (target != mt_uhf_mem_bank_OFF && (mask == NULL || mask_len_bit == 0))
        return -EINVAL;
    if (mask_len_bit && !mask)
        return -EINVAL;
    char cmd[128];
    int  ret = snprintf(cmd, sizeof(cmd), "AT+BMSK=%s", mt_uhf_get_membank_name(target));
    if (ret < 0)
        return ret;
    if (target != mt_uhf_mem_bank_OFF) {
        size_t offset = ret;
        ret           = snprintf(cmd + offset, sizeof(cmd) - offset, ",%u", mask_start_bit);
        if (ret < 0)
            return ret;
        offset += ret;
        if (offset + 2 >= sizeof(cmd))
            return -EINVAL;
        cmd[offset++]         = ',';
        size_t mask_len_bytes = (mask_len_bit + 7) / 8;
        if (sizeof(cmd) - offset < mask_len_bytes + 3)
            return -ENOBUFS;
        mt_print_hex(mask, mask_len_bytes, cmd + offset);
        offset += 2 * mask_len_bytes;
        ret = snprintf(cmd + offset, sizeof(cmd) - offset, ",%u", mask_len_bit);
        if (ret < 0)
            return ret;
        offset += ret;
        if (offset >= sizeof(cmd))
            return -EINVAL;
    }
    return mt_uhf_setter_call(cmd, 0);
}

int mt_uhf_get_inventory_mask(enum mt_uhf_mem_bank *target,
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
        return -EINVAL;
    return mt_uhf_get_data("AT+BMSK?", mt_uhf_get_inventory_mask_cb, &usr_data, 0);
}

int mt_uhf_set_channel_mask(uint64_t mask)
{
    if (reader.state.region != mt_uhf_region_etsi_lowerband)
        return -ENAVAIL;
    if (mask > 0x0F) //there are only 4 channels in ETSI lower band
        return -EINVAL;
    char cmd[128];
    snprintf(cmd,
             sizeof(cmd),
             "AT+CMSK=%08X%08X",
             (uint32_t)(mask >> 32),
             (uint32_t)(mask & UINT32_MAX));
    return mt_uhf_setter_call(cmd, 0);
}

static int mt_uhf_get_channel_mask_cb(const char *prefix, char *data, bool finalized)
{
    if (!finalized || strcmp(prefix, "+CMSK") || !data)
        return -EBADMSG;
    size_t  data_len = strlen(data);
    uint8_t buffer[8];
    if (data_len == 0 || data_len > 16 || data_len % 2)
        return -EBADMSG;
    int ret = mt_parse_hex_array_to_bytes(data, data_len, true, buffer, 8);
    if (ret)
        return -EBADMSG;
    if (reader.cmd.usr_data) {
        uint64_t *target = reader.cmd.usr_data;
        *target          = 0;
        for (int i = 0; i < data_len / 2; i++)
            *target = *target * 256 + buffer[i];
    }
    return EXIT_SUCCESS;
}

int mt_uhf_get_channel_mask(uint64_t *mask)
{
    int      ret      = mt_uhf_get_data("AT+CMSK?", mt_uhf_get_channel_mask_cb, mask, 0);
    uint64_t mask_all = (1ULL << 50) - 1;
    if (ret == EXIT_SUCCESS && mask && *mask == mask_all)
        *mask = 0;
    return ret;
}
