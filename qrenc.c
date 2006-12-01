/**
 * qrencode - QR Code encoder
 *
 * QR Code encoding tool
 * Copyright (C) 2006 Kentaro Fukuchi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <getopt.h>

#include "qrencode.h"

static int kanji = 0;
static int version = 0;
static int size = 3;
static int margin = 4;
static QRecLevel level = QR_ECLEVEL_L;

enum {
	O_HELP,
	O_OUTPUT,
	O_SIZE,
	O_VERSION,
	O_LEVEL,
	O_MARGIN,
	O_INPUT,
	O_KANJI
};

const struct option options[] = {
	{"h", no_argument      , NULL, O_HELP},
	{"o", required_argument, NULL, O_OUTPUT},
	{"l", required_argument, NULL, O_LEVEL},
	{"s", required_argument, NULL, O_SIZE},
	{"v", required_argument, NULL, O_VERSION},
	{"m", required_argument, NULL, O_MARGIN},
	{"i", required_argument, NULL, O_INPUT},
	{"k", no_argument      , NULL, O_KANJI}
};

void usage(void)
{
	fprintf(stderr,
"Usage: qrencode [OPTION]...\n"
"  -h           : display this message\n"
"  -o <filename>: set the output file. If it is not specified, the result is\n"
"                 output to standard output\n"
"  -s <number>  : specify the size of a dot (pixel) (default=3)\n"
"  -v <number>  : specify the version of the symbol\n"
"  -m <number>  : specify the width of margin (default=4)\n"
"  -i <text>    : specify input text. If it s not specified, get input text\n"
"                 from standard input\n"
"  -k           : assume that the input text contains kanji (shift-jis)\n"
);
}

#define MAX_DATA_SIZE 7090 /* from the specification */
char *readStdin(void)
{
	char *buffer;
	int ret;

	buffer = (char *)malloc(MAX_DATA_SIZE);
	ret = fread(buffer, 1, MAX_DATA_SIZE, stdin);
	if(ret == 0) {
		fprintf(stderr, "No input data.\n");
		exit(1);
	}
	if(!feof(stdin)) {
		fprintf(stderr, "Input data is too large.\n");
		exit(1);
	}

	buffer[ret] = '\0';

	return buffer;
}

QRcode *encode(const char *intext)
{
	QRencodeMode hint;

	if(kanji) {
		hint = QR_MODE_KANJI;
	} else {
		hint = QR_MODE_8;
	}
	return QRcode_encodeString(intext, version, level, hint);
}

void qrencode(const char *intext, const char *outfile)
{
	FILE *fp;
	QRcode *qrcode;
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char *row, *p, *q;
	int x, y, xx, yy, bit;
	int realwidth;
	
	qrcode = encode(intext);
	if(qrcode == NULL) {
		fprintf(stderr, "Failed to encode the input data.\n");
		exit(1);
	}

	realwidth = (qrcode->width + margin * 2) * size;
	row = (unsigned char *)malloc((realwidth + 7) / 8);

	if(outfile != NULL) {
		fp = fopen(outfile, "w");
		if(fp == NULL) {
			fprintf(stderr, "Failed to create file: %s\n", outfile);
			exit(1);
		}
	} else {
		fp = stdout;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL) {
		fclose(fp);
		fprintf(stderr, "Failed to initialize PNG writer.\n");
		exit(1);
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL) {
		fclose(fp);
		fprintf(stderr, "Failed to initialize PNG write.\n");
		exit(1);
	}

	if(setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		fprintf(stderr, "Failed to write PNG image.\n");
		exit(1);
	}

	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr,
			realwidth, realwidth,
			1,
			PNG_COLOR_TYPE_GRAY,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	/* top margin */
	memset(row, 0xff, (realwidth + 7) / 8);
	for(y=0; y<margin * size; y++) {
		png_write_row(png_ptr, row);
	}

	/* data */
	p = qrcode->data;
	for(y=0; y<qrcode->width; y++) {
		bit = 7;
		memset(row, 0xff, (realwidth + 7) / 8);
		q = row;
		q += margin * size / 8;
		bit = 7 - (margin * size % 8);
		for(x=0; x<qrcode->width; x++) {
			for(xx=0; xx<size; xx++) {
				*q ^= (*p & 1) << bit;
				bit--;
				if(bit < 0) {
					q++;
					bit = 7;
				}
			}
			p++;
		}
		for(yy=0; yy<size; yy++) {
			png_write_row(png_ptr, row);
		}
	}
	/* bottom margin */
	memset(row, 0xff, (realwidth + 7) / 8);
	for(y=0; y<margin * size; y++) {
		png_write_row(png_ptr, row);
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
}

int main(int argc, char **argv)
{
	int opt;
	char *outfile = NULL;
	char *intext = NULL;

	while((opt = getopt_long_only(argc, argv, "", options, NULL)) != -1) {
		switch(opt) {
			case O_HELP:
				usage();
				exit(0);
				break;
			case O_OUTPUT:
				outfile = optarg;
				break;
			case O_SIZE:
				size = atoi(optarg);
				if(size <= 0) {
					fprintf(stderr, "Invalid size: %d\n", size);
					exit(1);
				}
				break;
			case O_VERSION:
				version = atoi(optarg);
				if(version < 0) {
					fprintf(stderr, "Invalid version: %d\n", version);
					exit(1);
				}
				break;
			case O_LEVEL:
				switch(*optarg) {
					case 'l':
					case 'L':
						level = QR_ECLEVEL_L;
						break;
					case 'm':
					case 'M':
						level = QR_ECLEVEL_M;
						break;
					case 'q':
					case 'Q':
						level = QR_ECLEVEL_Q;
						break;
					case 'h':
					case 'H':
						level = QR_ECLEVEL_H;
						break;
					default:
						fprintf(stderr, "Invalid level: %s\n", optarg);
						exit(1);
						break;
				}
				break;
			case O_MARGIN:
				margin = atoi(optarg);
				if(margin < 0) {
					fprintf(stderr, "Invalid margin: %d\n", margin);
					exit(1);
				}
				break;
			case O_INPUT:
				intext = optarg;
				break;
			case O_KANJI:
				kanji = 1;
				break;
			default:
				usage();
				exit(1);
				break;
		}
	}

	if(intext == NULL) {
		intext = readStdin();
	}

	qrencode(intext, outfile);

	return 0;
}
