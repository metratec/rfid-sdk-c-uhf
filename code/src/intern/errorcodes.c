/*
 * Filename: errorcodes.c
 * Path: code/src/intern/
 * Created Date: Friday, January 9th 2026, 3:08:45 pm
 * Author: Martin Koehler
 * 
 * Copyright (c) 2026 Metratec
 */

#include <metratec/uhf_reader/public/typedef.h>

const char *mt_uhf_error2string(mt_uhf_errorcode_t errorcode)
{
    if (errorcode > mt_uhf_errorcode_success)
        return "No error but a value above 0 returned";
    if (errorcode == mt_uhf_errorcode_success)
        return "No error";
    if (errorcode == mt_uhf_errorcode_general_fault)
        return "General fault";
    if (errorcode == mt_uhf_errorcode_busy_call_again)
        return "Busy, try again instantly (or later)";
    if (errorcode == mt_uhf_errorcode_invalid_parameter)
        return "A given parameter or parameter set is out of range";
    if (errorcode == mt_uhf_errorcode_memory_full)
        return "A given memory buffer is full";
    if (errorcode == mt_uhf_errorcode_no_buffer)
        return "No memory buffer was given or is available";
    if (errorcode == mt_uhf_errorcode_not_available)
        return "The function called (maybe dependend on parameter) is currently not available (likely caused by a setting)";
    if (errorcode == mt_uhf_errorcode_not_supported)
        return "The function is not supported on this device type";
    if (errorcode == mt_uhf_errorcode_no_data_available)
        return "No data available on interface, wait for more before retry";
    if (errorcode == mt_uhf_errorcode_timeout)
        return "Reader did not respond in time";
    if (errorcode == mt_uhf_errorcode_already)
        return "The reader is already set to the given value";
    if (errorcode == mt_uhf_errorcode_format_fault)
        return "The reader answer has a flawed format like missing start characters, wrong parameter count or format, unexpected event type and more";
    if (errorcode == mt_uhf_errorcode_range)
        return "The reader answer is out of the expected range and gets ignored to protect the sdk's integrity";
    if (errorcode == mt_uhf_errorcode_inconsistent)
        return "Function state or return value are outside the expected range, something went wrong";
    return "The errorcode is unknown";
}