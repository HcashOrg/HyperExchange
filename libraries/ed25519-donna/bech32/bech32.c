/**
 * By nullius <nullius@nym.zone>
 * PGP:		0xC2E91CD74A4C57A105F6C21B5A00591B2F307E0C
 * Bitcoin:	3NULL3ZCUXr7RDLxXeLPDMZDZYxuaYkCnG
 *		bc1qcash96s5jqppzsp8hy8swkggf7f6agex98an7h
 *
 * Copyright (c) 2017.  All rights reserved.
 *
 * The Antiviral License (AVL) v0.0.1, with added Bitcoin Consensus Clause:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of the source code must retain the above copyright
 *    and credit notices, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    and credit notices, this list of conditions, and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. Derivative works hereof MUST NOT be redistributed under any license
 *    containing terms which require derivative works and/or usages to
 *    publish source code, viz. what is commonly known as a "copyleft"
 *    or "viral" license.
 * 4. Derivative works hereof which have any functionality related to
 *    digital money (so-called "cryptocurrency") MUST EITHER adhere to
 *    consensus rules fully compatible with Bitcoin Core, OR use a name
 *    which does not contain the word "Bitcoin".
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef __linux__
#define	_POSIX_C_SOURCE	200809L
#endif

#include <sys/types.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <err.h>

#include "segwit_addr.h"

#define	MODE_DECODE		0
#define	MODE_ENCODE		1
#define	MODE_SEGWIT_DECODE	2
#define	MODE_SEGWIT_ENCODE	3
#define	MODE_ONION_DECODE	0x00012000
#define	MODE_ONION_ENCODE	0x00013000
#define	IO_HEX			0x10
#define	IO_RFC4648		0x20

static ssize_t
hexdec(unsigned char *data, size_t len, const char *hexdata, size_t hexdata_len)
{
	ssize_t data_len = 0;

	if (len < hexdata_len / 2 || (hexdata_len & 1))
		return (-1);

	for (size_t i = 0, j = 0; i < hexdata_len; i+=2, ++j) {
#ifdef notyet
	/* For copypaste of old-style PGP fingerprint display, etc. */
	Unfortunately, this musses the length checks in main() and
	the loop counting here.  General concept:

	if (isblank(hexdata[i]))
		continue;
#endif
		if (hexdata[i] >= '0' && hexdata[i] <= '9')
			data[j] = (hexdata[i] - '0') << 4;
		else if (hexdata[i] >= 'A' && hexdata[i] <= 'F')
			data[j] = (hexdata[i] - ('A' - 0xa)) << 4;
		else if (hexdata[i] >= 'a' && hexdata[i] <= 'f')
			data[j] = (hexdata[i] - ('a' - 0xa)) << 4;
		else
			return (-1);

		if (hexdata[i+1] >= '0' && hexdata[i+1] <= '9')
			data[j] |= hexdata[i+1] - '0';
		else if (hexdata[i+1] >= 'A' && hexdata[i+1] <= 'F')
			data[j] |= hexdata[i+1] - ('A' - 0xa);
		else if (hexdata[i+1] >= 'a' && hexdata[i+1] <= 'f')
			data[j] |= hexdata[i+1] - ('a' - 0xa);
		else
			return (-1);

		++data_len;
	}

	return (data_len);
}

ssize_t
hexenc(char *hd, size_t buflen, const unsigned char *data, size_t datalen, int cs)
{
	const char hex[2][16] = { "0123456789abcdef", "0123456789ABCDEF" };
	ssize_t hexlen = 0;

	if (buflen < datalen * 2 + 1)
		return (-1);

	cs = !!cs;

	for (int i = 0; i < datalen; ++i) {
		*hd++ = hex[cs][data[i] >> 4];
		*hd++ = hex[cs][data[i] & 0xf];
		hexlen += 2;
	}
	*hd = '\0';

	return (hexlen);
}

static ssize_t
b32enc(unsigned char *b32, size_t b32buflen, const unsigned char *data, size_t datalen)
{
	unsigned bits = 0, b32char = 0;
	ssize_t b32data_len = 0;

	if (b32buflen < (datalen * 8 / 5 + !!(datalen * 8 % 5)))
		return (-1);

	do {
		b32char <<= 8, b32char |= *data++, bits += 8;

		while (bits >= 5) {
			*b32++ = (b32char >> (bits - 5)), ++b32data_len;
			b32char &= ~(0x1f << (bits -= 5));
		}
	} while (--datalen > 0);

	assert(bits < 5);
	if (bits > 0)
		*b32 = b32char << (5 - bits), ++b32data_len;

	return (b32data_len);
}

static ssize_t
b32dec(unsigned char *data, size_t buflen, const unsigned char *b32data, size_t b32datalen)
{
	unsigned bits = 0, u8char = 0;
	ssize_t datalen = 0;

	do {
		if (*b32data > 0x1f)
			return (-1);

		u8char <<= 5, u8char |= *b32data++, bits += 5;
		while (bits >= 8) {
			*data++ = u8char >> (bits - 8), ++datalen;
			u8char &= ~(0xff << (bits -= 8));
		}
	} while (--b32datalen > 0);
	assert(bits <= 8); /* Guaranteed by logic. */
	if (bits > 4 || u8char != 0) /* Invalid per the specification. */
		return (-1);

	return (datalen);
}

static ssize_t
rfcb32enc(char *b32, const unsigned char *data, size_t datalen)
{
	const char alphabet[32] = "abcdefghijklmnopqrstuvwxyz234567";
	ssize_t len = 0;

	while (datalen > 0) {
		if (*data > 31)
			return (-1);
		*b32++ = alphabet[*data++], --datalen, ++len;
	}
	*b32 = '\0';
}

static ssize_t
rfcb32dec(unsigned char *data, size_t buflen, const char *b32, size_t b32len)
{
	ssize_t datalen = 0;

	while (b32len > 0) {
		if (*b32 >= 'a' && *b32 <= 'z')
			*data = *b32 - 'a';
		else if (*b32 >= 'A' && *b32 <= 'Z')
			*data = *b32 - 'A';
		else if (*b32 >= '2' && *b32 <= '7')
			*data = *b32 - ('2' - 26);
		else
			return (-1);

		++b32, ++data, ++datalen, --b32len;
	}

	return (datalen);
}

int
main(int argc, char *argv[])
{
	int ch, error, mode = -1, b32mode = 8, hexcase = 0;
	long witver = -1;
	const char *hrp = NULL, *hexdata = NULL, *bechdata = NULL, *cur;
	char bech32[256], codehrp[84], *str = bech32, *endptr;
	unsigned char data[80], b32data[128], *b32cur = b32data;
	ssize_t hrp_len, data_len = 0, b32data_len = 0, hexdata_len;

	while ((ch = getopt(argc, argv, "58Sdeh:s:u")) > -1) {
		switch (ch) {
		case '5':
			b32mode = 5;
			break;
		case '8': /* default */
			b32mode = 8;
			break;
		case 'S':
			mode = MODE_SEGWIT_DECODE;
			break;
		case 'd':
			mode = MODE_DECODE;
			break;
		case 'e':
			mode = MODE_ENCODE;
			break;
		case 'h':
			hrp = optarg;
			break;
		case 's':
			mode = MODE_SEGWIT_ENCODE;
			errno = 0;
			witver = strtol(optarg, &endptr, 10);
			if (witver < 0 || witver > 16 ||
				*endptr != '\0' || errno != 0)
				witver = -2;
			break;
		case 'u':
			hexcase = 1;
			break;
		default:
			errx(1, "Bad option");
		}
	}
	argc -= optind, argv += optind;

	if (*argv == NULL || *(argv+1) != NULL)
		errx(1, "Wrong options");

	if (witver == -2)
		errx(1, "Invalid witness version provided with -s");

	if (witver != -1 && hrp != NULL)
		errx(1, "-h hrp is automatically set by -s option");

	if (witver != -1 && mode != MODE_SEGWIT_ENCODE)
		errx(1, "Mode conflict");

	switch (mode) {
	case MODE_ENCODE:
		if ((cur = strchr(*argv, '.')) != NULL) {
			if (strcmp(cur, ".onion") != 0)
				errx(1, "Bad encoding string");
			hrp = "onion";
			hrp_len = strlen(hrp);
			hexdata = *argv;
			hexdata_len = cur - *argv;
			b32mode = 5;
			mode = MODE_ONION_ENCODE;
			goto onion_encode;
		} else {
			if (hrp == NULL)
				errx(1, "HRP needed (-h hrp)");
			hrp_len = strlen(hrp);
			if (hrp_len < 1 || hrp_len > 83)
				errx(1, "Bad HRP length");
		}

		/* FALLTHRUOGH */
	case MODE_SEGWIT_ENCODE:
		hexdata = *argv;
		hexdata_len = strlen(hexdata);
		if (hexdata_len < 6 || (hexdata_len & 1))
			errx(1, "Bad hex data length");

		if (strncmp(hexdata, "0x", 2) == 0) {
			hexdata += 2, hexdata_len -= 2;
			if (hexdata_len < 6)
				errx(1, "Bad hex data length");
		}

		/* XXX: This leaves the real check to bech32_encode(). */
		if (hexdata_len > sizeof(data) * 2)
			errx(1, "Bad hex data length");

		if (mode == MODE_SEGWIT_ENCODE)
			goto segwit_encode;

onion_encode:
		if (b32mode == 5) {
			b32data_len = rfcb32dec(b32data, sizeof(b32data), hexdata, hexdata_len);
			if (b32data_len < 0)
				errx(1, "Bad base32 data");
		} else {
			data_len = hexdec(data, sizeof(data), hexdata, hexdata_len);
			if (data_len < 0)
				errx(1, "Bad hex data");

			b32data_len = b32enc(b32data, sizeof(b32data), data, data_len);
			if (b32data_len < 0)
				errx(1, "base32 encoding failed");
		}

		error = bech32_encode(bech32, hrp, b32data, b32data_len);
		if (error != 1)
			errx(1, "bech32_encode() failed (data_len: %ju)",
				(intmax_t)data_len);

		printf("%s\n", bech32);
		break;
segwit_encode:
		data_len = hexdec(data, sizeof(data), hexdata, hexdata_len);
		if (data_len < 0)
			errx(1, "Bad hex data");
fprintf(stderr, "data_len: %jd\n", (intmax_t)data_len);
		error = segwit_addr_encode(bech32, "bc", witver, data, data_len);
		if (error != 1)
			errx(1, "segwit_addr_encode() failed");

		printf("%s\n", bech32);
		break;
	case MODE_DECODE:
		error = bech32_decode(codehrp, b32data, &b32data_len, *argv);
		if (error != 1)
			errx(1, "bech32_decode() failed");

		if (!strcmp(codehrp, "onion"))
			b32mode = 5, mode = MODE_ONION_DECODE;

		if (b32mode == 5) {
			rfcb32enc(str, b32data, b32data_len);
			if (mode == MODE_ONION_DECODE)
				printf("%s.onion\n", str);
			else
				printf("%s:%s\n", codehrp, str);
		} else {
			data_len = b32dec(data, sizeof(data), b32data, b32data_len);
			hexenc(str, sizeof(bech32), data, data_len, hexcase);

			printf("%s:0x%s\n", codehrp, str);
		}
		break;
	case MODE_SEGWIT_DECODE: {
		int ver;

		error = segwit_addr_decode(&ver, data, &data_len, "bc", *argv);
		if (error != 1)
			errx(1, "segwit_addr_decode() failed");

		hexenc(str, sizeof(bech32), data, data_len, hexcase);

		printf("%d:0x%s\n", ver, str);

		break;
	}
	default:
		errx(1, "Mode not set (encode or decode)");
	}

	return (0);
}
