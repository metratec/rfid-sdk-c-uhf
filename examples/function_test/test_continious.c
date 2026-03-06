//C
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//Interface
#include <interface.h>
#include <serial.h>

//Metratec
#include <metratec/uhf_reader/intern/reader.h>
#include <metratec/uhf_reader_sdk.h>

static bool test_at(void)
{
    mt_uhf_errorcode_t ret = mt_uhf_setter_call("AT", 0);
    return ret == mt_uhf_errorcode_success;
}
static bool test_identification(void)
{
    struct mt_uhf_reader_identification id;
    mt_uhf_errorcode_t                  ret = mt_uhf_get_identification(&id);
    if (ret != mt_uhf_errorcode_success)
        return false;
    return true;
}

static int                    state    = 0;
static struct mt_uhf_gen2_tag test_tag = { .count = 0 };
static bool                   _tag_cb(struct mt_uhf_gen2_tag *tagp)
{
    if (!tagp) {
        printf("Round finished, got %u tags\n", state);
        state *= -1;
        return true; //No data, no buffering
    }
    printf("Tag found, EPC: ");
    for (int i = 0; i < tagp->epc.fill; i++) {
        if ((i & 3) == 0)
            putchar(' ');
        printf("%02X", tagp->epc.data[i]);
    }
    if (tagp->tid.fill) {
        printf(", TID:");
        for (int i = 0; i < tagp->tid.fill; i++) {
            if ((i & 3) == 0)
                putchar(' ');
            printf("%02X", tagp->tid.data[i]);
        }
    }
    printf(", %u times\n", tagp->count);
    printf("RSSI: %i, Phase: %i / %i\n", tagp->rssi, tagp->phase[0], tagp->phase[1]);
    if (test_tag.count == 0) {
        test_tag = *tagp;
        printf("Tag saved for masking\n");
    }

    state--;
    return true;
}

static bool test_inv(void)
{
    mt_uhf_errorcode_t ret = mt_uhf_inventory(_tag_cb, NULL, 5000);
    if (ret < 0)
        printf("Inventory returned error %d: %s \n", ret, mt_uhf_error2string(ret));
    printf("INV done: %i tags found\n", -1 * state);
    state = 0;
    return (ret == mt_uhf_errorcode_success);
}
static uint8_t rw_data[64];
static uint8_t rw_data_init = 0;
static bool    _test_read(void)
{
    if (test_tag.count == 0)
        return true;

    if (rw_data_init < sizeof(rw_data)) {
        struct mt_uhf_buffer answer_buffer = { .fill = 0,
                                               .size = 8,
                                               .data = rw_data + rw_data_init };
        unsigned int         read_errors   = 0;

        mt_uhf_errorcode_t ret = mt_uhf_read_data(mt_uhf_mem_bank_USR,
                                                  8,
                                                  rw_data_init,
                                                  &answer_buffer,
                                                  NULL,
                                                  1,
                                                  &test_tag.epc,
                                                  &read_errors,
                                                  0);

        if (ret == 1 && read_errors == 0 && answer_buffer.fill == 8) {
            rw_data_init += 8;
            return true;
        }
        if (ret < mt_uhf_errorcode_success)
            printf("Read returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        return false;
    }
    uint8_t data[16];
    int     offset  = (rand() % (sizeof(rw_data) / 2)) * 2; //offset needs to be even
    int     len     = (rand() % sizeof(data)) + 1;
    int     max_len = sizeof(rw_data) - offset;
    if (len > max_len)
        len = max_len;
    struct mt_uhf_buffer answer_buffer = { .fill = 0, .size = sizeof(data), .data = data };
    unsigned             read_errors   = 0;
    mt_uhf_errorcode_t   ret           = mt_uhf_read_data(
        mt_uhf_mem_bank_USR, len, offset, &answer_buffer, NULL, 1, &test_tag.epc, &read_errors, 0);
    if (ret < mt_uhf_errorcode_success || read_errors) {
        printf("Read returned error %d: %s with %u read errors\n",
               ret,
               mt_uhf_error2string(ret),
               read_errors);
        return false;
    }
    if (ret == 0) {
        printf("Read failed to find the tag\n");
        return true;
    }
    if (ret != 1)
        printf("Read found to many tags: %i\n", ret);
    if (answer_buffer.fill != len || 0 != memcmp(rw_data + offset, data, len)) {
        printf("Unexpected data state\n");
        return false;
    }
    return true;
}
static bool _test_write(void)
{
    if (test_tag.count == 0)
        return true;
    if (rw_data_init < sizeof(rw_data))
        return true;

    uint8_t data[16];
    int     offset  = (rand() % (sizeof(rw_data) / 2)) * 2; //offset needs to be even
    int     len     = ((rand() % (sizeof(data) / 2)) + 1) * 2;
    int     max_len = sizeof(rw_data) - offset;
    if (len > max_len)
        len = max_len;
    for (int i = 0; i < len; i++)
        data[i] = rand() & 0xFF;
    unsigned           write_errors = 0;
    mt_uhf_errorcode_t ret          = mt_uhf_write_data(
        mt_uhf_mem_bank_USR, data, len, offset, NULL, 0, &test_tag.epc, &write_errors, 0);
    if (ret < mt_uhf_errorcode_success || write_errors) {
        printf("Write returned error %d: %s and %u tag errors\n",
               ret,
               mt_uhf_error2string(ret),
               write_errors);
        if (ret == mt_uhf_errorcode_invalid_parameter) {
            printf("Arguments: data: %p, len: %i, %i, NULL, 0, &test_tag.epc\n", data, len, offset);
        } else {
            //got executed but failed, read data back
            do {
                struct mt_uhf_buffer answer_buffer = { .fill = 0,
                                                       .size = sizeof(data),
                                                       .data = data };
                ret                                = mt_uhf_read_data(mt_uhf_mem_bank_USR,
                                       len,
                                       offset,
                                       &answer_buffer,
                                       NULL,
                                       1,
                                       &test_tag.epc,
                                       NULL,
                                       0);
            } while (ret < 0);
            memcpy(rw_data + offset, data, len);
        }
        return false;
    }
    if (ret == 0) { //amount of tags
        printf("Write failed to find the tag\n");
        return true;
    }
    if (ret != 1)
        printf("Write found to many tags: %i\n", ret);

    uint8_t              read_data[16];
    struct mt_uhf_buffer answer_buffer = { .fill = 0,
                                           .size = sizeof(read_data),
                                           .data = read_data };
    ret                                = mt_uhf_read_data(
        mt_uhf_mem_bank_USR, len, offset, &answer_buffer, NULL, 1, &test_tag.epc, NULL, 0);
    if (ret < mt_uhf_errorcode_success) {
        printf("Readback returned error %d: %s \n", ret, mt_uhf_error2string(ret));
        if (ret == mt_uhf_errorcode_invalid_parameter) {
            printf("Arguments: len: %i, %i\n", len, offset);
        }
        return false;
    }
    if (ret == 0) {
        printf("Readback failed to find the tag\n");
        return true;
    }
    if (ret != 1) {
        printf("Readback found to many tags: %i\n", ret);
        return false;
    }
    if (answer_buffer.fill != len || 0 != memcmp(read_data, data, len)) {
        printf("Readback unexpected data state, reset tag mirror data\n");
        rw_data_init = 0;
        return false;
    }
    memcpy(rw_data + offset, data, len);
    return true;
}

static bool _test_echo(void)
{
    int                echo = rand() % 2;
    mt_uhf_errorcode_t ret  = mt_uhf_reader_echo_set(!!echo);
    if (ret < 0)
        printf("Echo returned error %d: %s \n", ret, mt_uhf_error2string(ret));
    return ret == mt_uhf_errorcode_success;
}

typedef bool (*test_function_wrapper_t)(void);
struct test_descriptor {
    test_function_wrapper_t function;
    const char             *name;
    unsigned                priority_multiplier;
} tests[] = {
    { test_at, "Handshake", 10 },  { test_identification, "Identification", 10 },
    { test_inv, "Inventory", 20 }, { _test_read, "Read", 20 },
    { _test_write, "Write", 2 },   { _test_echo, "Echo", 2 },
};

void test_random_function(void)
{
    time_t ltime;
    time(&ltime);
    static unsigned priority_sum = 0;
    if (priority_sum == 0)
        for (int i = 0; i < ARRAY_SIZE(tests); i++)
            priority_sum += tests[i].priority_multiplier;
    int random = rand() % priority_sum; //technically that's not exact but its close enough
    for (int i = 0; i < ARRAY_SIZE(tests); i++)
        if (random < tests[i].priority_multiplier) {
            bool ret = tests[i].function();
            if (!ret) {
                printf("Test %s failed at %s\n", tests[i].name, ctime(&ltime));
                for (int j = 0; j < 100; j++) {
                    (void)comm_update();
                    mt_cmd_wait(10);
                }
                at_rx_ring_flush();
            }
            return;
        } else
            random -= tests[i].priority_multiplier;
}