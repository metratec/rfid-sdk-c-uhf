/**
 * @file: posix_serial.c                                                                           *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-04-16                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 */

//POSIX
#include <posix_interface.h>

//METRATEC
#include <metratec/uhf_reader_sdk.h>

static void _serial_cb(int status);
static void _serial_retry_cb(int status);

struct serial {
    int              port;
    struct itimerval retry_timer;
    size_t           retry_time_ms;
    struct sigaction irq_setting;
    struct sigaction irq_setting_retry;
    bool             irq_running;
} static serial = { .port = -1 };

static void _sio_cb(int status)
{
    // printf("SIGIO irq with status %i\n", status);
}

int comm_start(char *port_name, bool use_irq, size_t rx_time)
{
    if (!port_name || rx_time >= 1000)
        return -EINVAL;
    int ret = open(port_name, O_RDWR);
    if (ret < EXIT_SUCCESS)
        return ret;
    serial.port = ret;
    struct termios tty;
    ret = tcgetattr(serial.port, &tty);
    if (ret != EXIT_SUCCESS)
        return ret;

    if (use_irq) {
        serial.irq_setting.sa_handler        = _serial_cb;
        serial.irq_setting.sa_flags          = 0;
        serial.irq_setting.sa_restorer       = NULL;
        serial.irq_setting_retry.sa_handler  = _serial_retry_cb;
        serial.irq_setting_retry.sa_flags    = 0;
        serial.irq_setting_retry.sa_restorer = NULL;
        sigaction(SIGIO, &serial.irq_setting, NULL);
        sigaction(SIGALRM, &serial.irq_setting_retry, NULL);
    } else {
        serial.irq_setting.sa_handler  = _sio_cb;
        serial.irq_setting.sa_flags    = 0;
        serial.irq_setting.sa_restorer = NULL;
        sigaction(SIGIO, &serial.irq_setting, NULL);
    }
    ret = fcntl(serial.port, F_SETOWN, getpid());
    if (ret) {
        printf("fcntl failed: %i - %s\n", errno, strerror(errno));
        exit(ret);
    }
    /* Make the file descriptor asynchronous (the manual page says only 
           O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
    ret = fcntl(serial.port, F_SETFL, O_ASYNC);
    if (ret) {
        printf("fcntl failed: %i - %s\n", errno, strerror(errno));
        exit(ret);
    }

    tty.c_cflag     = CS8 | CLOCAL | CREAD;
    tty.c_iflag     = IGNPAR;
    tty.c_oflag     = 0;
    tty.c_lflag     = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = use_irq ? 1 : (rx_time / 100);

    ret = cfsetispeed(&tty, B115200);
    if (ret) {
        printf("cfsetispeed failed: %i - %s\n", errno, strerror(errno));
        exit(ret);
    }
    ret = tcflush(serial.port, TCIOFLUSH);
    if (ret) {
        printf("tcflush failed: %i - %s\n", errno, strerror(errno));
        exit(ret);
    }
    ret = tcsetattr(serial.port, TCSANOW, &tty);
    if (ret) {
        printf("tcsetattr failed: %i - %s\n", errno, strerror(errno));
        exit(ret);
    }
    serial.retry_time_ms = rx_time;
    serial.irq_running   = false;
    return EXIT_SUCCESS;
}

void comm_stop(void)
{
    if (serial.port >= 0)
        close(serial.port);
    serial.port = -1;
}
static void _serial_cb(int status)
{
    serial.retry_timer.it_value = (struct timeval){ 0, 0 };
    setitimer(ITIMER_REAL, &serial.retry_timer, NULL);

    if (serial.irq_running || serial.port < 0)
        return;
    serial.irq_running = true;

    int space = mt_rfid_reader_rx_remaining_empty();
    if (space == 0)
        goto retry;

    char   buf[128];
    size_t len = (space >= sizeof(buf)) ? sizeof(buf) : space;
    int    ret = read(serial.port, buf, len);
    if (ret < EXIT_SUCCESS)
        exit(-ECONNABORTED);
    mt_rfid_reader_rx(buf, ret);
    // printf("Serial read: %i / %lu\n", ret, len);
    int avail;
    int ret2 = ioctl(serial.port, FIONREAD, &avail);
    if (ret2) {
        printf("Avail failed %i - %s\n", ret2, strerror(-ret2));
        exit(-ECONNABORTED);
    }
    if (avail < 0)
        printf("Serial avail: %i\n", avail);
    if (avail <= 0)
        goto exit;
retry:
    serial.retry_timer.it_value = (struct timeval){ 0, serial.retry_time_ms * 1000 };
    setitimer(ITIMER_REAL, &serial.retry_timer, NULL);
exit:
    serial.irq_running = false;
}

static void _serial_retry_cb(int status)
{
    if (serial.irq_running || serial.port < 0)
        return;

    serial.irq_running = true;

    int space = mt_rfid_reader_rx_remaining_empty();
    if (space == 0)
        goto retry;

    char   buf[128];
    size_t len = (space >= sizeof(buf)) ? sizeof(buf) : space;
    int    ret = read(serial.port, buf, len);
    if (ret < EXIT_SUCCESS)
        exit(-ECONNABORTED);
    mt_rfid_reader_rx(buf, ret);

    int avail;
    ioctl(serial.port, FIONREAD, &avail);
    if (avail > 0)
        goto retry;
    serial.retry_timer.it_value = (struct timeval){ 0, 0 };
    setitimer(ITIMER_REAL, &serial.retry_timer, NULL);
    goto exit;

retry:
    serial.retry_timer.it_value = (struct timeval){ 0, serial.retry_time_ms * 1000 };
    setitimer(ITIMER_REAL, &serial.retry_timer, NULL);
exit:
    serial.irq_running = false;
}

int comm_update(void)
{
    if (serial.port < 0)
        return -ENODEV;
    if (serial.irq_setting.sa_handler == _serial_cb)
        return -EINVAL;
    int space = mt_rfid_reader_rx_remaining_empty();
    if (space == 0)
        return -ENOBUFS;
    char buf[128];
    int  len = (space >= sizeof(buf)) ? sizeof(buf) : space;
    int  ret = read(serial.port, buf, len);
    if (ret < EXIT_SUCCESS)
        return -ECONNABORTED;
    len = mt_rfid_reader_rx(buf, ret);
    if (len < ret)
        printf("Could only buffer %i out of %i bytes (even though %i bytes should be available)\n",
               len,
               ret,
               space);
    return len;
}

int mt_rfid_reader_tx(const uint8_t *data, size_t data_len)
{
    int ret = write(serial.port, data, data_len);
    return ret;
}