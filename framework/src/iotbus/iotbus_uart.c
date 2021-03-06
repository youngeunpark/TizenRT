/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/**
 * @file iotbus_uart.c
 * @brief Iotbus APIs for UART
 */

#include <tinyara/config.h>

#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <iotbus/iotbus_error.h>
#include <iotbus/iotbus_uart.h>

// debugging
#include <stdio.h>
#define zdbg printf   // adbg, idbg are already defined

struct _iotbus_uart_s {
	int fd;
};

int g_iotbus_uart_br[30] = {
B50, B75, B110, B134, B150,
B200, B300, B600, B1200, B1800,
B2400, B4800, B9600, B19200, B38400,
B57600, B115200, B128000, B230400, B256000,
B460800, B500000, B576000, B921600, B1000000,
B1152000, B1500000, B2000000, B2500000, B3000000, };
int g_iotbus_uart_size = 30;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private Functions
 */

int _iotbus_valid_baudrate(unsigned int rate)
{
	int i = 0;
	for (; i < g_iotbus_uart_size; i++)
		if (rate == g_iotbus_uart_br[i])
			return 1;

	return 0;
}
/*
 * Public Functions
 */

iotbus_uart_context_h iotbus_uart_init(const char *path)
{
	struct _iotbus_uart_s *handle = NULL;
	int fd;
	fd = open(path, O_RDWR, 0666);
	if (fd < 0)
		return handle;

	handle = (struct _iotbus_uart_s *)malloc(sizeof(struct _iotbus_uart_s));
	handle->fd = fd;

	return handle;
}

int iotbus_uart_stop(iotbus_uart_context_h hnd)
{
	if (!hnd)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	close(hnd->fd);

	return IOTBUS_ERROR_NONE;
}

int iotbus_uart_flush(iotbus_uart_context_h hnd)
{
#ifndef CONFIG_SERIAL_TERMIOS
	return IOTBUS_ERROR_NOT_SUPPORTED;
#else
	// tcdrain is not working even CONFIG_SERIAL_TERMIOS is defined.

	if (!hnd)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	int fd = hnd->fd;
	int ret = tcflush(fd, TCIOFLUSH);
	if (ret < 0)
		return IOTBUS_ERROR_NOT_SUPPORTED;

	return IOTBUS_ERROR_NONE;
#endif
}

int iotbus_uart_set_baudrate(iotbus_uart_context_h hnd, unsigned int baud)
{
#ifndef CONFIG_SERIAL_TERMIOS
	return IOTBUS_ERROR_NOT_SUPPORTED;
#else
	if (!hnd)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	if (!_iotbus_valid_baudrate(baud))
		return IOTBUS_ERROR_INVALID_PARAMETER;

	int fd = hnd->fd;
	int ret;
	struct termios tio;

	ret = tcgetattr(fd, &tio);
	if (ret)
		return IOTBUS_ERROR_UNKNOWN;

	tio.c_speed = baud;

	ret = tcsetattr(fd, TCSANOW, &tio);
	if (ret)
		return IOTBUS_ERROR_UNKNOWN;

	return IOTBUS_ERROR_NONE;
#endif
}

// 8N1 would be set by calling "iotbus_uart_set_mode(hnd, 8,MRAA_UART_PARITY_NONE , 1)"
int iotbus_uart_set_mode(iotbus_uart_context_h hnd, int bytesize, iotbus_uart_parity_e parity, int stopbits)
{
#ifndef CONFIG_SERIAL_TERMIOS
	return IOTBUS_ERROR_NOT_SUPPORTED;
#else
	if (!hnd)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	int fd = hnd->fd;
	int ret;
	struct termios tio;
	int byteinfo[4] = { CS5, CS6, CS7, CS8 };

	ret = tcgetattr(fd, &tio);
	if (ret)
		return IOTBUS_ERROR_UNKNOWN;

	// set byte size
	if (bytesize < 5 || bytesize > 8)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= byteinfo[bytesize - 5];

	// set parity info
	switch (parity) {
	case IOTBUS_UART_PARITY_EVEN:
		tio.c_cflag |= PARENB;
		tio.c_cflag &= ~PARODD;
		break;
	case IOTBUS_UART_PARITY_ODD:
		tio.c_cflag |= PARENB;
		tio.c_cflag |= PARODD;
		break;
	case IOTBUS_UART_PARITY_NONE:
		tio.c_cflag &= ~PARENB;
		tio.c_cflag &= ~PARODD;
		break;
	default:
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	// set stop bit
	switch (stopbits) {
	case 1:
		tio.c_cflag &= ~CSTOPB;
		break;
	case 2:
		tio.c_cflag |= CSTOPB;
	default:
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	ret = tcsetattr(fd, TCSANOW, &tio);
	if (ret)
		return IOTBUS_ERROR_UNKNOWN;

	return IOTBUS_ERROR_NONE;
#endif
}

// parameter
// rtscts => 1: rts/cts on, 0: off
int iotbus_uart_set_flowcontrol(iotbus_uart_context_h hnd, int xonxoff, int rtscts)
{
#ifndef CONFIG_SERIAL_TERMIOS
	return IOTBUS_ERROR_NOT_SUPPORTED;
#else
	if (!hnd)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	if (xonxoff != 1 && xonxoff != 0)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	if (rtscts != 1 && rtscts != 0)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	int fd = hnd->fd;
	int ret;
	struct termios tio;

	ret = tcgetattr(fd, &tio);
	if (ret)
		return IOTBUS_ERROR_UNKNOWN;

	if (rtscts)
		tio.c_cflag |= CRTS_IFLOW | CCTS_OFLOW;
	else
		tio.c_cflag &= ~(CRTS_IFLOW | CCTS_OFLOW);

	if (xonxoff)
		tio.c_iflag |= (IXON | IXOFF | IXANY);

	ret = tcsetattr(fd, TCSANOW, &tio);
	if (ret)
		return IOTBUS_ERROR_UNKNOWN;

	return IOTBUS_ERROR_NONE;
#endif
}

int iotbus_uart_read(iotbus_uart_context_h hnd, char *buf, unsigned int length)
{
	if (!hnd)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	if (!buf)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	if (length <= 0)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	int fd = hnd->fd;
	int ret = read(fd, buf, length);
	if (ret < 0)
		return IOTBUS_ERROR_UNKNOWN;

	return ret;
}

int iotbus_uart_write(iotbus_uart_context_h hnd, const char *buf, unsigned int length)
{
	if (!hnd)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	if (!buf)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	if (length <= 0)
		return IOTBUS_ERROR_INVALID_PARAMETER;

	int fd = hnd->fd;
	int ret = write(fd, buf, length);
	if (ret < 0)
		return IOTBUS_ERROR_UNKNOWN;
//    return IOTBUS_ERROR_NONE;
	return ret;
}

#ifdef __cplusplus
}
#endif
