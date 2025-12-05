/**
 * @file: inventory.c                                                                              *
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

DEFINE_PREFIX(INV);
DEFINE_PREFIX(CINV);
DEFINE_PREFIX(INVR);
DEFINE_PREFIX(CINVR);
DEFINE_PREFIX(MINV);
DEFINE_PREFIX(CMINV);
DEFINE_PREFIX(TID);

static inline void _buffer_tag(struct mt_uhf_inventory_buffer *inv_buffer);
static int         _parse_inv_data_info(char *data, size_t len);
static int         _parse_inv_data_tag(char *data, struct mt_uhf_gen2_tag *tag, bool reported);
static int         _inventory_inv_data_cb(const char *prefix, char *data, bool finalized);
static int         _inventory_invr_data_cb(const char *prefix, char *data, bool finalized);

int mt_uhf_inventory_stop(uint32_t timeout_ms)
{
    char                                cmd[32];
    struct mt_uhf_frame_data_cb_lookup *lu;
    if ((lu = mt_uhf_framehandler_get_data_cb(prefix_CINV)) && lu->cb)
        snprintf(cmd, sizeof(cmd), "AT+BINV");
    else if ((lu = mt_uhf_framehandler_get_data_cb(prefix_CINVR)) && lu->cb)
        snprintf(cmd, sizeof(cmd), "AT+BINVR");
    else if ((lu = mt_uhf_framehandler_get_data_cb(prefix_CMINV)) && lu->cb)
        snprintf(cmd, sizeof(cmd), "AT+BMINV");
    else
        return -ENAVAIL;

    int ret = mt_uhf_setter_call(cmd, timeout_ms);
    if (ret == EXIT_SUCCESS) {
        reader.resp.running_cinv = false;
        lu->cb                   = NULL;
    }
    return ret;
}

int mt_uhf_inventory(mt_uhf_tag_cb cb, struct mt_uhf_inventory_buffer *buffer, uint32_t timeout_ms)
{
    if (reader.state.inv_format == uhfv2_inventory_tag_data_format_unknown)
        return -ENAVAIL;
    if (buffer) {
        buffer->tags.fill  = 0;
        buffer->tags.found = 0;
        buffer->antenna    = 0;
    }

    reader.tag.cb = cb;
    int ret       = mt_uhf_get_data("AT+INV", _inventory_inv_data_cb, buffer, timeout_ms);
    reader.tag.cb = NULL;
    return ret;
}
int mt_uhf_inventory_automux(mt_uhf_tag_cb                   cb,
                             struct mt_uhf_inventory_buffer *buffer,
                             uint32_t                        timeout_ms)
{
    if (reader.state.inv_format == uhfv2_inventory_tag_data_format_unknown)
        return -ENAVAIL;
    if (buffer) {
        buffer->tags.fill  = 0;
        buffer->tags.found = 0;
        buffer->antenna    = 0;
    }

    reader.tag.cb = cb;
    int ret       = mt_uhf_get_data("AT+MINV", _inventory_inv_data_cb, buffer, timeout_ms);
    reader.tag.cb = NULL;
    return ret;
}
struct tid_usr_data {
    struct mt_uhf_buffer_tid *buffer;
    uint32_t                  size;
    uint32_t                  fill;
};
static int _inventory_read_tid_cb(const char *prefix, char *data, bool finalized)
{
    mt_rfid_reader_assert(data && prefix, "Missing data or prefix");
    mt_rfid_reader_assert(!strcmp(prefix_TID, prefix), "Wrong prefix");
    struct tid_usr_data *target = reader.cmd.usr_data;
    mt_rfid_reader_assert(target && target->size && target->buffer, "No TID user data");

    if (!data)
        return -EBADMSG;
    if (!strcmp(data, "<NO TAGS FOUND>")) {
        mt_rfid_reader_assert(target->fill == 0, "There should be no tags");
        return EXIT_SUCCESS;
    }
    size_t tid_len = strlen(data);
    //TID consists of multiples of 2 bytes -> 4 chars
    if (tid_len % 4 || tid_len / 2 > sizeof(target->buffer->data))
        return -EBADMSG;
    int ret = EXIT_SUCCESS;
    if (target->fill < target->size) {
        struct mt_uhf_buffer_tid *tid = &target->buffer[target->fill];
        ret       = mt_parse_hex_array_to_bytes(data, tid_len, true, tid->data, sizeof(tid->data));
        tid->fill = MT_UHF_GEN2_MAX_TID_BYTES;
    }
    target->fill++;
    return ret;
    //TID parsed
}

int mt_uhf_inventory_read_tid(struct mt_uhf_buffer_tid TIDs[],
                              uint32_t                 tid_size,
                              uint32_t                 timeout_ms)
{
    struct tid_usr_data usr_data = { .buffer = TIDs, .size = tid_size, .fill = 0 };
    int ret = mt_uhf_get_data("AT+TID", _inventory_read_tid_cb, &usr_data, timeout_ms);
    if (ret)
        return ret;
    return usr_data.fill;
}

static int _inventory_inv_data_cb(const char *prefix, char *data, bool finalized)
{
    mt_rfid_reader_assert(data && prefix, "Missing data or prefix");
    bool continious;
    if (!strcmp(prefix_INV, prefix) || !strcmp(prefix_MINV, prefix))
        continious = false;
    else if (!strcmp(prefix_CINV, prefix) || !strcmp(prefix_CMINV, prefix))
        continious = true;
    else
        return -EBADMSG;

    if (continious && finalized)
        reader.resp.state = uhf_resp_state_start;

    // Parse INV line and save tag information in internal buffer
    if (!data)
        return -EBADMSG;
    size_t len = strlen(data);
    int    ret;
    if (len >= 2 && data[0] == '<' && data[len - 1] == '>')
        ret = _parse_inv_data_info(data + 1, len - 2);
    else
        ret = _parse_inv_data_tag(data, &reader.tag.last, false);

    struct mt_uhf_inventory_buffer *inv_buffer;
    inv_buffer = continious ? reader.tag.inv_buf : mt_uhf_get_usr_data();
    if (inv_buffer && continious && inv_buffer->antenna) {
        //known antenna so the last round was done, reset the antenna and length
        inv_buffer->antenna   = 0;
        inv_buffer->tags.fill = inv_buffer->tags.found = 0;
    }
    if (ret == 0) { //Parsed a tag successfully
        bool put_to_buffer = true;
        if (reader.tag.cb)
            put_to_buffer = reader.tag.cb(&reader.tag.last);
        if (put_to_buffer)
            _buffer_tag(inv_buffer);
        return EXIT_SUCCESS; //Wait for next line
    }
    if (ret == -ENODATA) {   //info that no tag is in answer
        return EXIT_SUCCESS; //Wait for next line
    }
    if (ret < 0)
        return ret;
    //else round ended, got antenna number
    reader.tag.antenna = ret;
    if (inv_buffer)
        inv_buffer->antenna = reader.tag.antenna;
    if (reader.tag.cb)
        reader.tag.cb(NULL);
    if (continious && reader.cinv_round_cb)
        reader.cinv_round_cb(inv_buffer);
    return EXIT_SUCCESS;
}

int mt_uhf_inventory_reported(mt_uhf_tag_cb                   cb,
                              struct mt_uhf_inventory_buffer *buffer,
                              uint32_t                        run_time_ms,
                              uint32_t                        timeout_ms)
{
    if (timeout_ms < 100 + run_time_ms)
        return -EINVAL;
    if (reader.state.inv_format == uhfv2_inventory_tag_data_format_unknown)
        return -ENAVAIL;
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "AT+INVR=%u", run_time_ms);

    if (buffer) {
        buffer->tags.fill  = 0;
        buffer->tags.found = 0;
        buffer->antenna    = 0;
    }
    reader.tag.cb = cb;
    int ret       = mt_uhf_get_data(cmd, _inventory_invr_data_cb, buffer, timeout_ms);
    reader.tag.cb = NULL;
    return ret;
}

int mt_uhf_inventory_reported_start_continious(
    mt_uhf_tag_cb                   cb,
    struct mt_uhf_inventory_buffer *buffer,
    void(round_done_cb)(struct mt_uhf_inventory_buffer *inv_buf),
    uint32_t run_time_ms)
{
    if (reader.state.inv_format == uhfv2_inventory_tag_data_format_unknown)
        return -ENAVAIL;
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CINVR=%u", run_time_ms);
    reader.resp.running_cinv = true;
    int ret                  = mt_uhf_setter_call(cmd, 0);
    if (ret == EXIT_SUCCESS) {
        reader.cinv_round_cb = round_done_cb;
        ret                = mt_uhf_framehandler_set_data_cb(prefix_CINVR, _inventory_invr_data_cb);
        reader.resp.state  = uhf_resp_state_start;
        reader.tag.cb      = cb;
        reader.tag.inv_buf = buffer;
        if (buffer) {
            buffer->tags.fill  = 0;
            buffer->tags.found = 0;
            buffer->antenna    = 0;
        }
    }
    return ret;
}

int mt_uhf_inventory_start_continious(mt_uhf_tag_cb                   cb,
                                      struct mt_uhf_inventory_buffer *buffer,
                                      void(round_done_cb)(struct mt_uhf_inventory_buffer *inv_buf))
{
    if (reader.state.inv_format == uhfv2_inventory_tag_data_format_unknown) {
        mt_uhf_get_inventory_settings(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        if (reader.state.inv_format == uhfv2_inventory_tag_data_format_unknown)
            return -ENAVAIL;
    }
    reader.resp.running_cinv = true;
    int ret                  = mt_uhf_setter_call("AT+CINV", 0);
    if (ret == EXIT_SUCCESS) {
        reader.cinv_round_cb = round_done_cb;
        ret                  = mt_uhf_framehandler_set_data_cb(prefix_CINV, _inventory_inv_data_cb);
        reader.resp.state    = uhf_resp_state_start;
        reader.tag.cb        = cb;
        reader.tag.inv_buf   = buffer;
        if (buffer) {
            buffer->tags.fill  = 0;
            buffer->tags.found = 0;
            buffer->antenna    = 0;
        }
    }
    return ret;
}

int mt_uhf_inventory_automux_start_continious(
    mt_uhf_tag_cb                   cb,
    struct mt_uhf_inventory_buffer *buffer,
    void(round_done_cb)(struct mt_uhf_inventory_buffer *inv_buf))
{
    if (reader.state.inv_format == uhfv2_inventory_tag_data_format_unknown) {
        mt_uhf_get_inventory_settings(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        if (reader.state.inv_format == uhfv2_inventory_tag_data_format_unknown)
            return -ENAVAIL;
    }
    int ret                  = mt_uhf_setter_call("AT+CMINV", 0);
    reader.resp.running_cinv = true;
    if (ret == EXIT_SUCCESS) {
        reader.cinv_round_cb = round_done_cb;
        ret                = mt_uhf_framehandler_set_data_cb(prefix_CMINV, _inventory_inv_data_cb);
        reader.tag.cb      = cb;
        reader.tag.inv_buf = buffer;
        if (buffer) {
            buffer->tags.fill  = 0;
            buffer->tags.found = 0;
            buffer->antenna    = 0;
        }
    }
    return ret;
}

static int _inventory_invr_data_cb(const char *prefix, char *data, bool finalized)
{
    mt_rfid_reader_assert(data && prefix, "Missing data or prefix");
    bool cinvr = !strcmp(prefix_CINVR, prefix), invr = !strcmp(prefix_INVR, prefix);
    mt_rfid_reader_assert(cinvr ^ invr, "Wrong prefix"); //needs to be exactly one of them
    if (cinvr && finalized)
        reader.resp.state = uhf_resp_state_start;

    if (!data)
        return -EBADMSG;
    size_t                          len = strlen(data);
    int                             ret;
    struct mt_uhf_inventory_buffer *inv_buffer = invr ? mt_uhf_get_usr_data() : reader.tag.inv_buf;

    if (len < 2 || data[0] != '<' || data[len - 1] != '>') {     //can't be info format
        ret = _parse_inv_data_tag(data, &reader.tag.last, true); //so it should be a tag
        if (ret < EXIT_SUCCESS)
            return ret;
        //Parsed a tag successfully
        bool put_to_buffer = true;
        if (reader.tag.cb)
            put_to_buffer = reader.tag.cb(&reader.tag.last);
        if (put_to_buffer)
            _buffer_tag(inv_buffer);
        return EXIT_SUCCESS;
    }

    ret = _parse_inv_data_info(data + 1, len - 2);
    if (ret == EXIT_SUCCESS) { //Round finished
        if (!cinvr)            //only continious has rounds
            return -EBADMSG;
        if (inv_buffer)
            inv_buffer->antenna = reader.tag.antenna;
        reader.resp.state = uhf_resp_state_start;
        if (reader.tag.cb)
            reader.tag.cb(NULL);
        if (reader.cinv_round_cb)
            reader.cinv_round_cb(inv_buffer);
        return EXIT_SUCCESS;
    }
    if (ret == -ENODATA) {       //info that no tag is in answer
        if (cinvr ^ finalized)   //its either CINVR (expecting round end) or finished
            return EXIT_SUCCESS; //Wait for next line
        return -EBADMSG;
    }
    if (ret > 0) //Antenna count, should never happen in (C)INVR
        return -EBADMSG;
    return ret;
}

static int _parse_inv_data_info(char *data, size_t len)
{
    mt_rfid_reader_assert(data && len, "No data to parse");
    const char INV_ROUND_FINISH_ANTENNA[]    = "ROUND FINISHED, ANT=";
    const char INV_ROUND_FINISH_NO_ANTENNA[] = "ROUND FINISHED";
    const char INV_NO_TAGS[]                 = "NO TAGS FOUND";
    const char REPORT_FINISHED[]             = "REPORT FINISHED";

    //so there is at least one byte for the antenna number
    if (len >= sizeof(INV_ROUND_FINISH_ANTENNA) &&
        !memcmp(data, INV_ROUND_FINISH_ANTENNA, sizeof(INV_ROUND_FINISH_ANTENNA) - 1))
    {
        size_t pre_antennna_len = (sizeof(INV_ROUND_FINISH_ANTENNA) - 1);
        size_t antenna_len      = len - pre_antennna_len;
        if (antenna_len > 2 || antenna_len == 0)
            return -EBADMSG;
        uint8_t v = data[pre_antennna_len] - '0';
        if (v > 9)
            return -EBADMSG;
        uint8_t antenna = v;
        if (antenna_len == 2) {
            v = data[pre_antennna_len + 1] - '0';
            if (v > 9)
                return -EBADMSG;
            antenna = antenna * 10 + v;
        }
        if (antenna == 0)
            return -EBADMSG;
        return antenna;
    }
    if ((len == (sizeof(INV_ROUND_FINISH_NO_ANTENNA) - 1)) &&
        !memcmp(data, INV_ROUND_FINISH_NO_ANTENNA, sizeof(INV_ROUND_FINISH_NO_ANTENNA) - 1))
    {
#if MAX_ANTENNAS_MUXED > 1
        return -EBADMSG;
#endif
        return 1;
    }
    if ((len == (sizeof(INV_NO_TAGS) - 1)) && !memcmp(data, INV_NO_TAGS, sizeof(INV_NO_TAGS) - 1)) {
        //no tags
        return -ENODATA;
    }
    if ((len == (sizeof(REPORT_FINISHED) - 1)) &&
        !memcmp(data, REPORT_FINISHED, sizeof(REPORT_FINISHED) - 1))
    {
        return 0;
    }
    //any other error
    return -EFAULT;
}

static int _parse_inv_data_tag(char *data, struct mt_uhf_gen2_tag *tag, bool reported)
{
    mt_rfid_reader_assert(reader.state.inv_format != uhfv2_inventory_tag_data_format_unknown,
                          "Inventory settings unknown");
    enum uhfv2_inventory_tag_data_format todo = reader.state.inv_format;
    //no phase info in reported inv
    if (reported)
        todo &= ~uhfv2_inventory_tag_data_format_Phase;
    tag->epc.fill = 0;
    tag->tid.fill = 0;
    tag->count    = 1;
    //first is always EPC
    const char *next_delim = strchr(data, ','); //will be used for all elements

    if (todo & uhfv2_inventory_tag_data_format_epc) { //name space
        size_t epc_len = next_delim ? (next_delim - data) : strlen(data);
        if (epc_len % 4) //epc consists of multiples of 2 bytes -> 4 chars
            return -EBADMSG;
        int ret =
            mt_parse_hex_array_to_bytes(data, epc_len, true, tag->epc.data, sizeof(tag->epc.data));
        if (ret)
            return ret;
        tag->epc.fill = epc_len / 2;
        todo &= ~uhfv2_inventory_tag_data_format_epc;
        //EPC parsed
    }
    if (!next_delim)
        return (todo) ? -EBADMSG : EXIT_SUCCESS;

    if (todo & uhfv2_inventory_tag_data_format_TID) {
        //expect TID
        //TID is more or less formatted like an EPC but has a min length (and not sure about max)
        const char *TID = next_delim + 1;
        next_delim      = strchr(TID, ',');
        size_t tid_len  = next_delim ? (next_delim - TID) : strlen(TID);
        if (tid_len % 4) //TID consists of multiples of 2 bytes -> 4 chars
            return -EBADMSG;
        int ret =
            mt_parse_hex_array_to_bytes(TID, tid_len, true, tag->tid.data, sizeof(tag->tid.data));
        if (ret)
            return ret;
        tag->tid.fill = tid_len / 2;
        todo &= ~uhfv2_inventory_tag_data_format_TID;
        //TID parsed
    }
    if (!next_delim)
        return (todo) ? -EBADMSG : EXIT_SUCCESS;

    if (todo & uhfv2_inventory_tag_data_format_RSSI) {
        //expect RSSI
        //RSSI is one int value with usually negative value
        const char *RSSI = next_delim + 1;
        next_delim       = strchr(RSSI, ',');
        size_t rssi_len  = next_delim ? (next_delim - RSSI) : strlen(RSSI);

        int value, ret;
        if ((ret = mt_parse_int(RSSI, rssi_len, &value)))
            return ret;
        tag->rssi = value;
        todo &= ~uhfv2_inventory_tag_data_format_RSSI;
        //RSSI parsed
    }
    if (!next_delim)
        return (todo) ? -EBADMSG : EXIT_SUCCESS;

    if (todo & uhfv2_inventory_tag_data_format_Phase) {
        //expect phase data
        //Phase is two int values (can be negative?)
        //get both raw values first
        const char *phase[2] = { next_delim + 1 };
        next_delim           = strchr(phase[0], ',');
        if (!next_delim)
            return -EBADMSG;
        size_t phase_len[2] = { next_delim - phase[0], 0 };
        phase[1]            = next_delim + 1;
        next_delim          = strchr(phase[1], ',');
        phase_len[1]        = next_delim ? (next_delim - phase[1]) : strlen(phase[1]);

        for (int p = 0; p < 2; p++) {
            int v, ret = mt_parse_int(phase[p], phase_len[p], &v);
            if (ret)
                return ret;
            tag->phase[p] = v;
        }
        todo &= ~uhfv2_inventory_tag_data_format_Phase;
        //Phase parsed
    }
    if (!next_delim)
        return (reported) ? -EBADMSG : EXIT_SUCCESS;

    if (reported) {
        //expect a report (inventory count), 1 unsigned int
        const char *report = { next_delim + 1 };
        next_delim         = strchr(report, ',');
        size_t report_len  = next_delim ? (next_delim - report) : strlen(report);

        int v, ret = mt_parse_int(report, report_len, &v);
        if (ret)
            return ret;
        if (v < 0)
            return -EBADMSG;
        tag->count = v;
        reported   = false;
    }
    return (todo || next_delim || reported) ? -EBADMSG : EXIT_SUCCESS;
}

static inline void _buffer_tag(struct mt_uhf_inventory_buffer *inv_buffer)
{
    if (!inv_buffer)
        return;
    mt_rfid_reader_assert(inv_buffer->antenna == 0, "Unexpected antenna state during inventory");
    mt_rfid_reader_assert(inv_buffer->tags.found >= inv_buffer->tags.fill &&
                              inv_buffer->tags.fill <= inv_buffer->tags.size,
                          "Tag buffering fill error");

    inv_buffer->tags.found++;
    if (!inv_buffer->tags.size || inv_buffer->tags.fill >= inv_buffer->tags.size - 1)
        return;
    memcpy(
        inv_buffer->tags.buffer + inv_buffer->tags.fill, &reader.tag.last, sizeof(reader.tag.last));
    inv_buffer->tags.fill++;
}
