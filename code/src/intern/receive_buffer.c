/**
 * @file: receive_buffer.c                                                                         *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-23                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */
#include <string.h>

#include <metratec/uhf_reader/intern/receive_buffer.h>
#include <metratec/uhf_reader/public/errorcodes.h>

struct at_rx_ring_data {
    size_t   get;
    size_t   put;
    size_t   size;
    size_t   fill;
    uint8_t *buffer;
    //this shows how many bytes after put already searched for the element delimiter
    size_t delimiter_checked; // must be 0 <= delimiter_checked <= fill
    bool   new_data;
} _data;

mt_uhf_errorcode_t at_rx_ring_init(void *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size < 64)
        return mt_uhf_errorcode_invalid_parameter;
    _data.buffer = buffer;
    _data.size   = buffer_size;
    at_rx_ring_flush();
    return mt_uhf_errorcode_success;
}

mt_uhf_errorcode_t mt_rfid_reader_rx(void *data, size_t data_len)
{
    if (data_len > mt_uhf_errorcode_success_max)
        return mt_uhf_errorcode_invalid_parameter;
    if (data_len == 0)
        return mt_uhf_errorcode_success; // nothing to do

#if DEBUG_PRINTOUT_PRINTF
    bool newline = false;
    printf(">> ");
    char *d = data;
    for (int i = 0; i < data_len; i++) {
        if (newline)
            printf("> ");
        newline = false;
        if (d[i] == '\r') {
            printf("\\r\n");
            newline = true;
        } else if (d[i] == '\n') {
            printf("\\n\n");
            newline = true;
        } else {
            printf("%c", d[i]);
        }
    }
    printf("\n");
#endif

    return at_rx_ring_push(data, data_len);
}

bool at_rx_new_data(void)
{
    bool new       = _data.new_data;
    _data.new_data = false;
    return new;
}

size_t mt_rfid_reader_rx_remaining_empty(void)
{
    mt_rfid_reader_assert(_data.size >= _data.fill, "Fill overflow");
    if (_data.fill > _data.size)
        return 0;
    return _data.size - _data.fill;
}

static inline size_t _rb_position_add(size_t pos, size_t add)
{
    mt_rfid_reader_assert((add < _data.size && pos < _data.size), "Invalid use");
    size_t space = _data.size - pos;
    if (add < space)
        return pos + add;
    return add - space;
}

size_t at_rx_ring_push(const uint8_t *data, size_t len)
{
    mt_rfid_reader_assert(data || len == 0, "No data available");
    if (!data || len == 0)
        return 0;

    size_t space = mt_rfid_reader_rx_remaining_empty();
    mt_rfid_reader_assert(space <= _data.size && _data.fill <= _data.size,
                          "Invalid ring buffer state");
    if (space == 0)
        return 0;

    len            = min(len, space);
    _data.new_data = true;

    //space before wrap
    size_t space_no_wrap = _data.size - _data.put;
    if (len <= space_no_wrap) {
        //enough space, just copy
        memcpy(_data.buffer + _data.put, data, len);
    } else {
        //not enough space, first copy as much as there is space before wrap
        memcpy(_data.buffer + _data.put, data, space_no_wrap);
        //Then copy the rest to the start of buffer
        memcpy(_data.buffer, data + space_no_wrap, len - space_no_wrap);
    }
    _data.fill += len;
    _data.put = _rb_position_add(_data.put, len);
    return len;
}

void at_rx_ring_flush(void)
{
    _data.get = _data.put = 0;
    _data.fill = _data.delimiter_checked = 0;
    _data.new_data                       = false;
}

mt_uhf_errorcode_t at_rx_ring_getframe(char *target, size_t max_len, bool delete_oversized_frames)
{
    size_t frame_len = 0;
    while (1) {
        if (_data.fill == _data.delimiter_checked)
            return mt_uhf_errorcode_no_data_available;
        size_t current = _rb_position_add(_data.get, _data.delimiter_checked);
        if (_data.buffer[current] != mt_uhf_delimiter_char_frame_element) {
            _data.delimiter_checked++;
            continue;
        }
        //found an element
        frame_len = _data.delimiter_checked + 1;
        break;
    }

    //no follow up byte in buffer, so can't check the type. Element will be found again next try
    if (frame_len >= _data.fill)
        return frame_len == _data.size ? mt_uhf_errorcode_no_buffer :
                                         mt_uhf_errorcode_no_data_available;

    size_t follow = _rb_position_add(_data.get, frame_len);
    if (_data.buffer[follow] == mt_uhf_delimiter_char_frame) //it's also a frame end
        frame_len++; //the frame end is part of the frame element so add one byte

    if (frame_len > max_len) { //can't push it into target
        if (delete_oversized_frames) {
            _data.delimiter_checked = 0;
            _data.fill -= frame_len;
            _data.get = _rb_position_add(_data.get, frame_len);
        }
        // else don't remove frame, it's up the the owner
        return mt_uhf_errorcode_no_buffer;
    }
    if (frame_len > mt_uhf_errorcode_success_max)
        return mt_uhf_errorcode_general_fault;

    //push data on target
    //data before ring wrap
    size_t len_no_wrap = _data.size - _data.get;
    if (frame_len <= len_no_wrap) {
        //enough space, just copy
        memcpy(target, _data.buffer + _data.get, frame_len);
    } else {
        //not enough data pre wrap, first copy as much as there is before wrap
        memcpy(target, _data.buffer + _data.get, len_no_wrap);
        //Then copy the rest from the start of buffer
        memcpy(target + len_no_wrap, _data.buffer, frame_len - len_no_wrap);
    }
    _data.delimiter_checked = 0;
    _data.fill -= frame_len;
    _data.get = _rb_position_add(_data.get, frame_len);
    return frame_len;
}