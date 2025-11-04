/**
 * @file: reader.h                                                                                 *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-28                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

#pragma once

#include <metratec/uhf_reader/intern/framehandler.h>
#include <metratec/uhf_reader/public/typedef.h>

#define DEFINE_PREFIX(x) static const char prefix_##x[] = "+" #x

#if MT_UHF_MUX_MAX
#define MAX_ANTENNAS_MUXED ((MT_UHF_MUX_MAX) * (MT_UHF_ANTENNA_COUNT))
#else
#define MAX_ANTENNAS_MUXED (MT_UHF_ANTENNA_COUNT)
#endif

struct mt_uhf_frame_data_cb_lookup {
    const char      *id;
    mt_uhf_data_cb_t cb;
};
//Order: EPC, TID, RSSI, Phase (in 2 int)
//All values are bit values so they can be stacked
enum uhfv2_inventory_tag_data_format {
    uhfv2_inventory_tag_data_format_unknown = 0,
    uhfv2_inventory_tag_data_format_epc     = 0x01,
    uhfv2_inventory_tag_data_format_TID     = 0x02, //set by INVS
    uhfv2_inventory_tag_data_format_RSSI    = 0x04, //set by INVS
    uhfv2_inventory_tag_data_format_Phase   = 0x08, //set by INVS
};
enum uhf_resp_state { uhf_resp_state_start, uhf_resp_state_echo, uhf_resp_state_end };

struct uhfv2_reader {
    struct mt_uhf_reader_identification id;
    struct uhfv2_reader_state {
        mt_uhf_boolx_t                       echo_enabled;
        enum mt_uhf_rf_mode                  rf_mode;
        enum mt_uhf_region                   region;
        enum mt_uhf_session                  session;
        enum uhfv2_inventory_tag_data_format inv_format;
        mt_uhf_boolx_t                       only_new_tag;
        mt_uhf_boolx_t                       fast_start;
        enum mt_uhf_gen2_inventory_target    inv_target;
        enum mt_uhf_gen2_inventory_selecting inv_selected;
    } state;
    struct mt_uhf_frame_data_cb_lookup data_cb[MT_UHF_MAX_EVENT_TYPES];
    //unknown frame type, if not set uhfv2_framehandler returns -EBADMSG, else the return value of unknown_frame_cb
    int (*unknown_frame_cb)(const char *data);
    //used for unknown but valid data, usually for answer
    mt_uhf_data_cb_t unknown_data_cb;
    void (*cinv_round_cb)(struct mt_uhf_inventory_buffer *inv_buf);
    struct {
        mt_uhf_tag_cb                   cb;
        struct mt_uhf_gen2_tag          last;
        struct mt_uhf_inventory_buffer *inv_buf;
        uint8_t                         antenna;
    } tag;

    struct {
        int (*poll_cb)(void);
    } rx;

    struct {
        const char                        *data; //string containing the last command, used for echo
        uint32_t                           start_time;
        uint32_t                           timeout;
        struct mt_uhf_frame_data_cb_lookup rsp_data;
        void                              *usr_data;
        void (*blocking_cb)(void);
    } cmd;
    struct {
        struct {
            struct uhfv2_frame                  current, last;
            struct mt_uhf_frame_data_cb_lookup *last_data_match;
        } frame;
        enum uhf_resp_state state;
        bool                running_cinv;
        struct {
            uint32_t base;
            uint32_t cinv_running;
        } default_timeout;
    } resp;
};
struct mt_uhf_byte_array {
    uint8_t *data;
    size_t   data_len;
};

extern struct uhfv2_reader reader;
extern const char         *selected_strings[mt_uhf_gen2_inventory_selecting_length];

int   mt_uhf_setter_call(const char *cmd, uint32_t timeout);
void *mt_uhf_get_usr_data(void);
int   mt_uhf_get_data(const char *cmd, mt_uhf_data_cb_t data_cb, void *usr_data, uint32_t timeout);
int   mt_uhf_get_data_byte_array(const char               *name,
                                 struct mt_uhf_byte_array *usr_data,
                                 uint32_t                  timeout);

void            mt_uhf_set_polling(mt_uhf_poll_cb cb);
int             mt_uhf_response_resolve(const bool block);
int             mt_uhf_device_reset(void);
int             mt_update_reader_identification(void);
int             mt_uhf_reader_echo_set(bool enable);
mt_uhf_boolx_t *mt_uhf_reader_echo_get(void);
bool            mt_uhf_rf_mode_valid(enum mt_uhf_rf_mode mode);
int             mt_uhf_update_rf_mode(void);
int             mt_uhf_update_region(void);
const char     *mt_uhf_get_region_name(enum mt_uhf_region region);
bool            mt_uhf_mode_matches_region(enum mt_uhf_region region, enum mt_uhf_rf_mode rf_mode);
int             mt_uhf_update_session(void);
const char     *mt_uhf_get_membank_name(enum mt_uhf_mem_bank mem_bank);
int             mt_uhf_update_echo(void);

struct mt_uhf_invs_usr_data {
    bool                                 only_new_tags;
    bool                                 show_rssi;
    bool                                 read_tid;
    bool                                 fast_start;
    bool                                 show_phase;
    enum mt_uhf_gen2_inventory_selecting selected;
    enum mt_uhf_gen2_inventory_target    target;
    int                                  rssi_threshold;
};

int mt_uhf_get_inventory_setting_cb(const char *prefix, char *data, bool finalized);

struct mt_uhf_bmsk_usr_data {
    enum mt_uhf_mem_bank *target;
    uint32_t             *mask_start_bit;
    uint32_t             *mask_len_bit;
    uint8_t              *mask;
    size_t                mask_buffer_size;
};
int mt_uhf_get_inventory_mask_cb(const char *prefix, char *data, bool finalized);
