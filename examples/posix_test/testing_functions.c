/**
 * @file: testing_functions.c                                                                      *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-28                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

//First get the posix settings
#include <posix_interface.h>
//Metratec
#include "testing_functions.h"

//using this 2 functions for printout, any user CAN do the same or just print with your own functions
//both come from reader.h so including this internal header may also work fine but can
//give the autocompletion too much options with functions not intended for the user
const char *mt_uhf_get_region_name(enum mt_uhf_region region);
const char *mt_uhf_get_membank_name(enum mt_uhf_mem_bank mem_bank);

bool test_masking(void)
{
    int ret = mt_uhf_set_inventory_mask(mt_uhf_mem_bank_EPC, 8, "\x20\x91\x0D\xFA", 4 * 8 - 2);
    if (ret) {
        printf("Mask setting returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }

    enum mt_uhf_mem_bank target;
    uint32_t             bit_len, start_bit;
    uint8_t              mask[128];

    ret = mt_uhf_get_inventory_mask(&target, &start_bit, mask, sizeof(mask), &bit_len);
    if (ret) {
        printf("Mask getting returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    printf("Mask answer: \n");
    printf("Target: %u - %s\n", target, mt_uhf_get_membank_name(target));
    printf("Start: %u\n", start_bit);
    printf("Length: %u\n", bit_len);
    for (int i = 0; i < (bit_len + 7) / 8; i++)
        printf("%02X", mask[i]);
    putchar('\n');

    ret = mt_uhf_set_inventory_mask(mt_uhf_mem_bank_OFF, 0, NULL, 0);
    if (ret) {
        printf("Mask setting returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }

    ret = mt_uhf_get_inventory_mask(&target, &start_bit, mask, sizeof(mask), &bit_len);
    if (ret) {
        printf("Mask getting returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    printf("Mask answer: \n");
    printf("Target: %u - %s\n", target, mt_uhf_get_membank_name(target));
    printf("Start: %u\n", start_bit);
    printf("Length: %u\n", bit_len);
    for (int i = 0; i < (bit_len + 7) / 8; i++)
        printf("%02X", mask[i]);
    putchar('\n');
    return true;
}

bool test_region_mode(enum mt_uhf_region region, enum mt_uhf_rf_mode mode)
{
    int ret;
    //TODO: Add testing with invalid region, invalid mode, unmatched mode / region
    if ((ret = mt_uhf_set_rf_mode(region, mode))) {
        printf("Setting RF mode returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    enum mt_uhf_region  _region;
    enum mt_uhf_rf_mode _mode;
    if ((ret = mt_uhf_get_rf_mode(&_region, &_mode))) {
        printf("Getting RF mode returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    if (_region != region) {
        printf("Unexpected region\n");
        return false;
    }
    if (_mode != mode) {
        printf("Unexpected mode\n");
        return false;
    }
    return true;
}

bool test_device_info(void)
{
    struct mt_uhf_reader_identification *id = mt_uhf_get_identification();
    if (!id) {
        printf("No revision data\n");
        return false;
    }
    printf("Reader data:\nSW: %s ", id->fw_name);
    for (int i = 0; i < 4; i++)
        putchar(id->fw_rev[i]);
    printf("\nHW: %s ", id->hw_name);
    for (int i = 0; i < 4; i++)
        putchar(id->hw_rev[i]);
    printf("\nSerial: ");
    for (int i = 0; i < 16; i++)
        putchar(id->serial[i]);
    putchar('\n');
    return true;
}

bool test_session(enum mt_uhf_session session)
{
    int ret;
    if ((ret = mt_uhf_set_session(mt_uhf_session_max + 1)))
        printf("Setting invalid RF mode returned error %d: %s \n", -ret, strerror(-ret));
    typeof(session) _session;
    if ((ret = mt_uhf_get_session(&_session))) {
        printf("Getting invalid RF mode returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    printf("Default session: %i\n", _session);
    if ((ret = mt_uhf_set_session(session))) {
        printf("Setting invalid RF mode returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    //Resets session local to unknown
    if (-EINVAL != (ret = mt_uhf_set_session(mt_uhf_session_max + 1))) {
        printf("Setting invalid RF mode returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    if ((ret = mt_uhf_get_session(&_session))) {
        printf("Getting invalid RF mode returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    printf("Result session: %i\n", _session);
    return true;
}

bool test_mux_antenna(uint8_t *antennas, unsigned antenna_count)
{
    int     ret;
    uint8_t _multiplex[16];
    printf("Antenna count: %u\n", antenna_count);
    if (!antenna_count) {
        //No antenna, expect error answer leading to EFAULT error
        if ((ret = mt_uhf_get_multiplex_antennas(_multiplex, ARRAY_SIZE(_multiplex))) == -EFAULT)
            return true;
        printf("Getting invalid multiplex returned error %d: %s\n", -ret, strerror(-ret));
        return false;
    }
    if ((ret = mt_uhf_get_multiplex_antennas(_multiplex, ARRAY_SIZE(_multiplex))) < 0) {
        printf("Getting invalid multiplex returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    printf("MUX returned: ");
    for (int i = 0; i < ret; i++)
        printf("%u%c", _multiplex[i], (i == (ret - 1)) ? '\n' : ',');

    if ((ret = mt_uhf_set_multiplex_antennas(antennas, antenna_count)) < 0) {
        printf("Setting invalid multiplex returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    if ((ret = mt_uhf_get_multiplex_antennas(_multiplex, ARRAY_SIZE(_multiplex))) < 0) {
        printf("Getting invalid multiplex returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    printf("MUX returned: ");
    for (int i = 0; i < ret; i++)
        printf("%u%c", _multiplex[i], (i == (ret - 1)) ? '\n' : ',');
    printf("MUX expected: ");
    for (int i = 0; i < antenna_count; i++)
        printf("%u%c", antennas[i], (i == (ret - 1)) ? '\n' : ',');

    if (ret = mt_uhf_set_antenna(antennas[0]) < 0) {
        printf("Setting antenna returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    return true;
}

uint32_t   hbt_test_last;
static int _hbt_cb(const char *prefix, char *data, bool finalized)
{
    static uint32_t c = 0;
    if (data || !finalized)
        return -EBADMSG;
    if (!prefix || strcmp(prefix, "+HBT"))
        return -EBADMSG;
    hbt_test_last = mt_rfid_reader_get_time();
    printf("HBT #%u\n", ++c);
    return EXIT_SUCCESS;
}
bool test_invr(unsigned hbt_time, struct mt_uhf_inventory_buffer *inv_buffer, unsigned test_time)
{
    int ret = mt_reader_heartbeat_set(hbt_time, _hbt_cb);
    if (ret < 0) {
        printf("Heartbeat setting returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    hbt_test_last = mt_rfid_reader_get_time();
    printf("Test reported inv\n");

    uint32_t start_test = mt_rfid_reader_get_time();
    while (mt_rfid_reader_get_time() - start_test <= test_time) {
        if ((mt_rfid_reader_get_time() - hbt_test_last) > 10000) {
            printf("HBT timeout\n");
            ret = mt_reader_heartbeat_set(0, NULL);
            break;
        }
        inv_buffer->tags.fill = 0;
        ret                   = mt_uhf_inventory_reported(NULL, inv_buffer, 500, 5000);
        if (ret < 0) {
            printf("Inventory returned error %d: %s \n", -ret, strerror(-ret));
            goto exit;
        } else {
            printf("Inventory returned success with %u tags:\n", inv_buffer->tags.fill);
            for (int t = 0; t < inv_buffer->tags.fill; t++) {
                for (int b = 0; b < inv_buffer->tags.buffer[t].epc.fill; b += 2)
                    printf("%02X%02X ",
                           inv_buffer->tags.buffer[t].epc.data[b],
                           inv_buffer->tags.buffer[t].epc.data[b + 1]);
                putchar('\n');
            }
        }
    }
exit:
    if (hbt_time)
        ret = mt_reader_heartbeat_set(0, NULL);
    printf("INVR test without cb done\n---\n\n");
    return ret == 0;
}

bool test_cinvr(unsigned                        hbt_time,
                struct mt_uhf_inventory_buffer *inv_buffer,
                unsigned                        test_time,
                void (*round_done_cb)(struct mt_uhf_inventory_buffer *buf))
{
    uint32_t start_test = mt_rfid_reader_get_time();
    int      ret        = mt_reader_heartbeat_set(hbt_time, _hbt_cb);
    if (ret < 0) {
        printf("Heartbeat setting returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    hbt_test_last = mt_rfid_reader_get_time();
    ret = mt_uhf_inventory_reported_start_continious(NULL, inv_buffer, round_done_cb, 500);
    if (ret < 0) {
        printf("CINVR error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    printf("CINVR returned success\n");
    while (mt_rfid_reader_get_time() - start_test <= test_time) {
        if ((mt_rfid_reader_get_time() - hbt_test_last) > 10000) {
            printf("HBT timeout\n");
            ret = mt_reader_heartbeat_set(0, NULL);
            break;
        }
        ret = mt_uhf_resolve();
        if (ret == -EAGAIN || ret == -ENODATA)
            continue;
        if (ret == -EALREADY)
            printf("Event handled\n");
        else
            printf("Resolve returned error %d: %s \n", -ret, strerror(-ret));
    }
    ret = mt_reader_heartbeat_set(0, NULL);
    if (ret < 0) {
        printf("Heartbeat setting returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    ret = mt_uhf_inventory_stop(20000);
    if (ret < 0) {
        printf("Continious inv stop error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
exit:
    if (hbt_time)
        ret = mt_reader_heartbeat_set(0, NULL);
    printf("CINVR test done\n---\n\n");
    return ret == 0;
}

bool test_cinv(unsigned                        hbt_time,
               struct mt_uhf_inventory_buffer *inv_buffer,
               unsigned                        test_time,
               void (*round_done_cb)(struct mt_uhf_inventory_buffer *buf))
{
    uint32_t start_test = mt_rfid_reader_get_time();
    int      ret        = mt_reader_heartbeat_set(hbt_time, _hbt_cb);
    if (ret < 0) {
        printf("Heartbeat setting returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    hbt_test_last = mt_rfid_reader_get_time();
    start_test    = mt_rfid_reader_get_time();
    ret           = mt_uhf_inventory_start_continious(NULL, inv_buffer, round_done_cb);
    if (ret < 0) {
        printf("CINV error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    printf("CINV returned success\n");
    while (mt_rfid_reader_get_time() - start_test <= test_time) {
        ret = mt_uhf_resolve();
        if (ret == -EAGAIN || ret == -ENODATA)
            continue;
        if (ret == -EALREADY)
            printf("Event handled\n");
        else
            printf("Resolve returned error %d: %s \n", -ret, strerror(-ret));
    }
    ret = mt_uhf_inventory_stop(20000);
    if (ret < 0) {
        printf("Continious inv stop error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
exit:
    if (hbt_time)
        ret = mt_reader_heartbeat_set(0, NULL);
    printf("CINV test done\n---\n\n");
    return ret == 0;
}

bool test_cminv(unsigned                        hbt_time,
                struct mt_uhf_inventory_buffer *inv_buffer,
                unsigned                        test_time,
                void (*round_done_cb)(struct mt_uhf_inventory_buffer *buf))
{
    uint32_t start_test = mt_rfid_reader_get_time();
    int      ret        = mt_reader_heartbeat_set(hbt_time, _hbt_cb);
    if (ret < 0) {
        printf("Heartbeat setting returned error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    hbt_test_last = mt_rfid_reader_get_time();
    start_test    = mt_rfid_reader_get_time();
    ret           = mt_uhf_inventory_automux_start_continious(NULL, inv_buffer, round_done_cb);
    if (ret < 0) {
        printf("CMINV error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
    printf("CMINV returned success\n");
    while (mt_rfid_reader_get_time() - start_test <= test_time) {
        ret = mt_uhf_resolve();
        if (ret == -EAGAIN || ret == -ENODATA)
            continue;
        if (ret == -EALREADY)
            printf("Event handled\n");
        else
            printf("Resolve returned error %d: %s \n", -ret, strerror(-ret));
    }
    ret = mt_uhf_inventory_stop(20000);
    if (ret < 0) {
        printf("Continious mux inv stop error %d: %s \n", -ret, strerror(-ret));
        goto exit;
    }
exit:
    if (hbt_time)
        ret = mt_reader_heartbeat_set(0, NULL);
    printf("CMINV test done\n---\n\n");
    return ret == 0;
}

bool test_tid(void)
{
    struct mt_uhf_buffer_tid TIDs[2];
    int                      ret = mt_uhf_inventory_read_tid(TIDs, ARRAY_SIZE(TIDs), 0);
    if (ret < 0) {
        printf("TID inv stop error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    printf("TID result: %i tags:\n", ret);
    for (int tag = 0; tag < min(ARRAY_SIZE(TIDs), ret); tag++) {
        printf("%02X%02X %02X%02X %02X%02X %02X%02X %02X%02X\n",
               TIDs[tag].data[0],
               TIDs[tag].data[1],
               TIDs[tag].data[2],
               TIDs[tag].data[3],
               TIDs[tag].data[4],
               TIDs[tag].data[5],
               TIDs[tag].data[6],
               TIDs[tag].data[7],
               TIDs[tag].data[8],
               TIDs[tag].data[9]);
    }
    return true;
}

bool test_rw_masked(struct mt_uhf_gen2_tag *tag)
{
    printf("Write / Read data test\n");
    printf("Write data\n");
    uint8_t  wdata[8], rdata[8];
    uint32_t now = mt_rfid_reader_get_time();
    for (int i = 0; i < sizeof(wdata); i++)
        wdata[i] = tag->epc.data[i] ^ now;

    int      ret         = 0;
    unsigned error_count = 0;
    if (tag->epc.fill) {
        ret = mt_uhf_set_inventory_mask(mt_uhf_mem_bank_EPC, 0, tag->epc.data, tag->epc.fill * 8);
        if (ret < 0) {
            printf("Mask error %d: %s \n", -ret, strerror(-ret));
            return false;
        }
    } else if (tag->tid.fill) {
        ret = mt_uhf_set_inventory_mask(mt_uhf_mem_bank_TID, 0, tag->tid.data, tag->tid.fill * 8);
        if (ret < 0) {
            printf("Mask error %d: %s \n", -ret, strerror(-ret));
            return false;
        }
    } else {
        printf("Tag has neither EPC nor TID\n");
        return false;
    }
    struct mt_uhf_buffer_epc epc;
    ret = mt_uhf_write_data(
        mt_uhf_mem_bank_USR, wdata, sizeof(wdata), 0, &epc, 1, NULL, &error_count);
    if (ret < 0 || error_count) {
        printf("Write error %d: %s with %u tag errors\n", -ret, strerror(-ret), error_count);
        return false;
    }
    printf("Write found %i tags\n", ret);
    if (ret == 0) {
        printf("No readback triggered as write found no tag\n");
        return true;
    }

    printf("Read USR data\n");
    struct mt_uhf_buffer answer = { .data = rdata, .size = sizeof(rdata) };
    ret                         = mt_uhf_read_data(
        mt_uhf_mem_bank_USR, sizeof(rdata), 0, &answer, NULL, 1, &epc, &error_count);
    if (ret < 0 || error_count) {
        printf("Read error %d: %s with %u tag errors\n", -ret, strerror(-ret), error_count);
        return false;
    }
    printf("Read found %i tags\n", ret);
    if (memcmp(rdata, wdata, sizeof(wdata))) {
        printf("Read data missmatch\n");
        return false;
    }
    printf("Readback matched\n");
    return true;
}

bool test_outputs(void)
{
#if MT_UHF_GPO_COUNT
    mt_uhf_output_states_t out = {
        mt_uhf_boolx_false, mt_uhf_boolx_false, mt_uhf_boolx_false, mt_uhf_boolx_false
    };
    int ret = mt_uhf_set_output(out);
    if (ret < 0) {
        printf("Out error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    for (int i = 0; i < (1 << MT_UHF_GPO_COUNT); i++) {
        for (int o = 0; o < MT_UHF_GPO_COUNT; o++) {
            mt_uhf_boolx_t new = i & (1 << o) ? mt_uhf_boolx_true : mt_uhf_boolx_false;
            out[o]             = (new == out[o]) ? mt_uhf_boolx_none : new;
        }
        ret = mt_uhf_set_output(out);
        if (ret < 0) {
            printf("Out %i error %d: %s \n", i, -ret, strerror(-ret));
            return false;
        }
        mt_cmd_wait(100);
    }
#endif
    return true;
}

#if MT_UHF_GPI_COUNT
bool test_inputs(mt_uhf_input_states_t *expected)
{
    mt_uhf_input_states_t data;
    int                   ret = mt_uhf_get_input(data);
    if (ret) {
        printf("mt_uhf_get_input() returned error %d: %s \n", -ret, strerror(-ret));
        return false;
    }
    if (memcmp(*expected, data, sizeof(data))) {
        printf("Input test failed, data do not match\n");
        return false;
    }
    return true;
}
#else
//intentional leave parameters open so anything put in is valid (and does not get used)
bool test_inputs()
{
    return true;
}
#endif
