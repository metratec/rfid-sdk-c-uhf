/**
 * @file: framehandler.c                                                                           *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-28                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */
#include <string.h>

#include <metratec/uhf_reader/intern/common.h>

static mt_uhf_errorcode_t _framehandler_dataframe(void);

mt_uhf_errorcode_t mt_uhf_current_frame_handled(void)
{
    mt_rfid_reader_assert(reader.resp.frame.last.data == NULL, "Last frame data was not handled?");

    reader.resp.frame.last    = (struct uhfv2_frame){ reader.resp.frame.current.type, NULL };
    reader.resp.frame.current = (struct uhfv2_frame){ uhfv2_frametype_none, NULL };
    return mt_uhf_errorcode_already; //this is needed by calling functions to see the frame is already handled
}

mt_uhf_errorcode_t mt_uhf_framehandler(char *data, size_t data_len)
{
#if DEBUG_PRINTOUT_PRINTF
    printf("Frame @%u:\n", mt_rfid_reader_get_time());
    if (data_len == 2 && data[0] == '\r' && data[1] == '\n')
        printf("CRLF\n");
    else {
        for (int i = 0; i < data_len; i++)
            printf("%c", data[i]);
        printf("\n");
    }
#endif
    if (data_len < 2) //crlf OR +cr at least
        goto invalid;

    if (data[data_len - 1] == mt_uhf_delimiter_char_frame_element) {
        //frame not ending, only data events do that
        reader.resp.frame.current.type = uhfv2_frametype_data_element;
        data[data_len - 1]             = '\0';
    } else {
        mt_rfid_reader_assert(data[data_len - 1] == mt_uhf_delimiter_char_frame,
                              "Data is no frame");
        data_len -= 2;
        if (data[data_len] != mt_uhf_delimiter_char_frame_element)
            goto invalid;
        data[data_len] = '\0';
        if (data_len == 0)
            reader.resp.frame.current.type = uhfv2_frametype_answer_start;
        else if (data[0] == '+')
            reader.resp.frame.current.type = uhfv2_frametype_data_finish;
        else if (reader.cmd.data) {
            //running command, can expect echo, OK, ERROR (and data anyways)
            if (data_len == strlen(reader.cmd.data) && !memcmp(reader.cmd.data, data, data_len))
                reader.resp.frame.current.type = uhfv2_frametype_echo;
            else if (data_len == 2 && !memcmp(data, "OK", data_len))
                reader.resp.frame.current.type = uhfv2_frametype_answer_finish_success;
            else if (data_len == 5 && !memcmp(data, "ERROR", data_len))
                reader.resp.frame.current.type = uhfv2_frametype_answer_finish_failed;
            else
                reader.resp.frame.current.type = uhfv2_frametype_data_finish;
        } else if (reader.unexpected_frame_cb) {
            mt_uhf_errorcode_t ret = reader.unexpected_frame_cb(data);
            if (ret == mt_uhf_errorcode_no_data_available)
                goto invalid;
            else
                return ret;
        } else
            goto invalid;
    }
    reader.resp.frame.current.data = data;

    switch (reader.resp.frame.current.type) {
    //data events are handled here, via callbacks
    case uhfv2_frametype_data_element:
    case uhfv2_frametype_data_finish: {
        mt_uhf_errorcode_t ret = _framehandler_dataframe();
        if (ret == mt_uhf_errorcode_success)
            return mt_uhf_current_frame_handled();
        else if (ret != mt_uhf_errorcode_already) {
            reader.resp.frame.current.type = uhfv2_frametype_invalid;
            (void)mt_uhf_current_frame_handled();
        }
        return ret;
    }
    case uhfv2_frametype_answer_start:
        if (!reader.cmd.data && !reader.resp.running_cinv) //no answer expected
            goto invalid;
        if (reader.resp.frame.last.type == uhfv2_frametype_answer_start) {
#if IGNORE_DOUBLE_FRAME_START
            reader.resp.frame.current.data = NULL;
            return mt_uhf_current_frame_handled();
#endif
            goto invalid;
        }
        if (reader.resp.frame.last.type == uhfv2_frametype_echo)
            goto invalid;
        if (reader.resp.frame.last.type == uhfv2_frametype_data_element) //was incomplete
            goto invalid;
        return mt_uhf_errorcode_success;
    case uhfv2_frametype_echo:
        if (!reader.cmd.data) //no cmd, no echo
            goto invalid;
        if (reader.resp.frame.last.type != uhfv2_frametype_answer_start)
            goto invalid;
        return mt_uhf_errorcode_success;

    case uhfv2_frametype_answer_finish_success:
        if (!reader.cmd.data) //no cmd, no answer finish
            goto invalid;
        if (reader.resp.frame.last.type == uhfv2_frametype_data_element) //was incomplete
            goto invalid;
        return mt_uhf_errorcode_success;
    case uhfv2_frametype_answer_finish_failed:
        if (!reader.cmd.data) //no answer, no answer error
            goto invalid;
        if (reader.resp.frame.last.type == uhfv2_frametype_data_element) //was incomplete
            goto invalid;
        return mt_uhf_errorcode_success;
    default:
        goto invalid;
    }

invalid:
    reader.resp.frame.current.type = uhfv2_frametype_invalid;
    reader.resp.frame.current.data = NULL;
    (void)mt_uhf_current_frame_handled();
    return mt_uhf_errorcode_format_fault;
}

/**
 * @brief       Function to handle the current data frame
 * 
 * @return      POSIX error codes
 */
static mt_uhf_errorcode_t _framehandler_dataframe(void)
{
    struct uhfv2_frame *frame = &reader.resp.frame.current;
    mt_rfid_reader_assert(frame->type == uhfv2_frametype_data_element ||
                              frame->type == uhfv2_frametype_data_finish,
                          "No data frame");
    size_t len = strlen(frame->data);
    if (len < 2)
        return mt_uhf_errorcode_format_fault;
    char *prefix = frame->data;
    char *colon  = memchr(prefix, ':', len); //start of data
    char *data   = NULL;

    if (colon) {
        *colon = '\0';
        data   = colon + 1;
        while (*data == ' ' || *data == '\t')
            data++;
    } //else no data

    struct mt_uhf_frame_data_cb_lookup *data_cb = mt_uhf_framehandler_get_data_cb(prefix);
    if (data_cb) {
        //if it was an unfinished data round check if the data id matches
        if (reader.resp.frame.last.type == uhfv2_frametype_data_element)
            if (reader.resp.frame.last_data_match != data_cb)
                return mt_uhf_errorcode_inconsistent;
        //if it is an unfinished data round set data to check in next round
        if (frame->type == uhfv2_frametype_data_element)
            reader.resp.frame.last_data_match = data_cb;
        else if (frame->type == uhfv2_frametype_data_finish)
            reader.resp.frame.last_data_match = NULL;

        if (!data_cb->cb)
            return mt_uhf_errorcode_success; //found a match, but it's just actively unhandled
        mt_uhf_errorcode_t ret =
            data_cb->cb(prefix, data, frame->type == uhfv2_frametype_data_finish);
        if (ret == mt_uhf_errorcode_success)
            return mt_uhf_current_frame_handled();
        return ret;
    }

    //no match
    if (reader.unknown_data_cb)
        return reader.unknown_data_cb(prefix, data, frame->type == uhfv2_frametype_data_finish);
    return mt_uhf_errorcode_range;
}

struct mt_uhf_frame_data_cb_lookup *mt_uhf_framehandler_get_data_cb(const char *prefix)
{
    mt_rfid_reader_assert(prefix, "No prefix for lookup");
    if (!prefix)
        return NULL;
    if (reader.cmd.rsp_data.id && !strcmp(reader.cmd.rsp_data.id, prefix))
        return &reader.cmd.rsp_data;
    for (int i = 0; i < ARRAY_SIZE(reader.data_cb); i++)
        if (reader.data_cb[i].id && !strcmp(prefix, reader.data_cb[i].id))
            return &reader.data_cb[i];
    return NULL;
}

mt_uhf_errorcode_t mt_uhf_framehandler_set_data_cb(const char *prefix, mt_uhf_data_cb_t cb)
{
    if (!prefix || !strlen(prefix))
        return mt_uhf_errorcode_invalid_parameter;
    struct mt_uhf_frame_data_cb_lookup *data_cb = mt_uhf_framehandler_get_data_cb(prefix);
    if (data_cb) {
        data_cb->cb = cb;
        if (!cb)
            data_cb->id = NULL;
        return mt_uhf_errorcode_success;
    }
    if (!cb)
        return mt_uhf_errorcode_success;
    for (int i = 0; i < ARRAY_SIZE(reader.data_cb); i++) {
        if (reader.data_cb[i].id)
            continue;
        reader.data_cb[i].id = prefix;
        reader.data_cb[i].cb = cb;
        return mt_uhf_errorcode_success;
    }
    return mt_uhf_errorcode_no_buffer;
}