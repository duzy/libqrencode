/*
 * qrencode - QR-code encoder
 *
 * Copyright (C) 2006 Kentaro Fukuchi
 *
 * The following data / specifications are taken from
 * "Two dimensional symbol -- QR-code -- Basic Specification" (JIS X0510:2004)
 *  or
 * "Automatic identification and data capture techniques -- 
 *  QR Code 2005 bar code symbology specification" (ISO/IEC 18004:2006)
 *
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

#include "qrspec.h"

/******************************************************************************
 * Version and capacity
 *****************************************************************************/

/**
 * Table of the capacity of symbols
 * See Table 1 (pp.13) and Table 12-16 (pp.30-36), JIS X0510:2004.
 */
QRspec_Capacity qrspecCapacity[QRSPEC_VERSION_MAX + 1] = {
	{  0,    0, {   0,    0,    0,    0}},
	{ 21,   26, {   7,   10,   13,   17}}, // 1
	{ 25,   44, {  10,   16,   22,   28}},
	{ 29,   70, {  15,   26,   36,   44}},
	{ 33,  100, {  20,   36,   52,   64}},
	{ 37,  134, {  26,   48,   72,   88}}, // 5
	{ 41,  172, {  36,   64,   96,  112}},
	{ 45,  196, {  40,   72,  108,  130}},
	{ 49,  242, {  48,   88,  132,  156}},
	{ 53,  292, {  60,  110,  160,  192}},
	{ 57,  346, {  72,  130,  192,  224}}, //10
	{ 61,  404, {  80,  150,  224,  264}},
	{ 65,  466, {  96,  176,  260,  308}},
	{ 69,  532, { 104,  198,  288,  352}},
	{ 73,  581, { 120,  216,  320,  384}},
	{ 77,  655, { 132,  240,  360,  432}}, //15
	{ 81,  733, { 144,  280,  408,  480}},
	{ 85,  815, { 168,  308,  448,  532}},
	{ 89,  901, { 180,  338,  504,  588}},
	{ 93,  991, { 196,  364,  546,  650}},
	{ 97, 1085, { 224,  416,  600,  700}}, //20
	{101, 1156, { 224,  442,  644,  750}},
	{105, 1258, { 252,  476,  690,  816}},
	{109, 1364, { 270,  504,  750,  900}},
	{113, 1474, { 300,  560,  810,  960}},
	{117, 1588, { 312,  588,  870, 1050}}, //25
	{121, 1706, { 336,  644,  952, 1110}},
	{125, 1828, { 360,  700, 1020, 1200}},
	{129, 1921, { 390,  728, 1050, 1260}},
	{133, 2051, { 420,  784, 1140, 1350}},
	{137, 2185, { 450,  812, 1200, 1440}}, //30
	{141, 2323, { 480,  868, 1290, 1530}},
	{145, 2465, { 510,  924, 1350, 1620}},
	{149, 2611, { 540,  980, 1440, 1710}},
	{153, 2761, { 570, 1036, 1530, 1800}},
	{157, 2876, { 570, 1064, 1590, 1890}}, //35
	{161, 3034, { 600, 1120, 1680, 1980}},
	{165, 3196, { 630, 1204, 1770, 2100}},
	{169, 3362, { 660, 1260, 1860, 2220}},
	{173, 3532, { 720, 1316, 1950, 2310}},
	{177, 3706, { 750, 1372, 2040, 2430}} //40
};

int QRspec_getMaximumCodeLength(int version, QRenc_ErrorCorrectionLevel level)
{
	return qrspecCapacity[version].words - qrspecCapacity[version].ec[level];
}

int QRspec_getECCLength(int version, QRenc_ErrorCorrectionLevel level)
{
	return qrspecCapacity[version].ec[level];
}

int QRspec_getMinimumVersion(int size, QRenc_ErrorCorrectionLevel level)
{
	int i;
	int words;

	for(i=1; i<= QRSPEC_VERSION_MAX; i++) {
		words  = qrspecCapacity[i].words - qrspecCapacity[i].ec[level];
		if(words >= size) return i;
	}

	return -1;
}

/******************************************************************************
 * Length indicator
 *****************************************************************************/

static int lengthTableBits[4][3] = {
	{10, 12, 14},
	{ 9, 11, 13},
	{ 8, 16, 16},
	{ 8, 10, 12}
};

int QRspec_lengthIndicator(QRenc_EncodeMode mode, int version)
{
	int l;

	if(version <= 9) {
		l = 0;
	} else if(version <= 26) {
		l = 1;
	} else {
		l = 2;
	}

	return lengthTableBits[mode][l];
}

int QRspec_maximumWords(QRenc_EncodeMode mode, int version)
{
	int l;
	int bits;
	int words;

	if(version <= 9) {
		l = 0;
	} else if(version <= 26) {
		l = 1;
	} else {
		l = 2;
	}

	bits = lengthTableBits[mode][l];
	words = (1 << bits) - 1;
	if(mode == QR_MODE_KANJI) {
		words *= 2; // the number of bytes is required
	}

	return words;
}

/******************************************************************************
 * Error correction code
 *****************************************************************************/

/**
 * Table of the error correction code (Reed-Solomon block)
 * See Table 12-16 (pp.30-36), JIS X0510:2004.
 */
static int eccTable[QRSPEC_VERSION_MAX+1][4][2] = {
	{{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0}},
	{{ 1,  0}, { 1,  0}, { 1,  0}, { 1,  0}}, // 1
	{{ 1,  0}, { 1,  0}, { 1,  0}, { 1,  0}},
	{{ 1,  0}, { 1,  0}, { 2,  0}, { 2,  0}},
	{{ 1,  0}, { 2,  0}, { 2,  0}, { 4,  0}},
	{{ 1,  0}, { 2,  0}, { 2,  2}, { 2,  2}}, // 5
	{{ 2,  0}, { 4,  0}, { 4,  0}, { 4,  0}},
	{{ 2,  0}, { 4,  0}, { 2,  4}, { 4,  1}},
	{{ 2,  0}, { 2,  2}, { 4,  2}, { 4,  2}},
	{{ 2,  0}, { 3,  2}, { 4,  4}, { 4,  4}},
	{{ 2,  2}, { 4,  1}, { 6,  2}, { 6,  2}}, //10
	{{ 4,  0}, { 1,  4}, { 4,  4}, { 3,  8}},
	{{ 2,  2}, { 6,  2}, { 4,  6}, { 7,  4}},
	{{ 4,  0}, { 8,  1}, { 8,  4}, {12,  4}},
	{{ 3,  1}, { 4,  5}, {11,  5}, {11,  5}},
	{{ 5,  1}, { 5,  5}, { 5,  7}, {11,  7}}, //15
	{{ 5,  1}, { 7,  3}, {15,  2}, { 3, 13}},
	{{ 1,  5}, {10,  1}, { 1, 15}, { 2, 17}},
	{{ 5,  1}, { 9,  4}, {17,  1}, { 2, 19}},
	{{ 3,  4}, { 3, 11}, {17,  4}, { 9, 16}},
	{{ 3,  5}, { 3, 13}, {15,  5}, {15, 10}}, //20
	{{ 4,  4}, {17,  0}, {17,  6}, {19,  6}},
	{{ 2,  7}, {17,  0}, { 7, 16}, {34,  0}},
	{{ 4,  5}, { 4, 14}, {11, 14}, {16, 14}},
	{{ 6,  4}, { 6, 14}, {11, 16}, {30,  2}},
	{{ 8,  4}, { 8, 13}, { 7, 22}, {22, 13}}, //25
	{{10,  2}, {19,  4}, {28,  6}, {33,  4}},
	{{ 8,  4}, {22,  3}, { 8, 26}, {12, 28}},
	{{ 3, 10}, { 3, 23}, { 4, 31}, {11, 31}},
	{{ 7,  7}, {21,  7}, { 1, 37}, {19, 26}},
	{{ 5, 10}, {19, 10}, {15, 25}, {23, 25}}, //30
	{{13,  3}, { 2, 29}, {42,  1}, {23, 28}},
	{{17,  0}, {10, 23}, {10, 35}, {19, 35}},
	{{17,  1}, {14, 21}, {29, 19}, {11, 46}},
	{{13,  6}, {14, 23}, {44,  7}, {59,  1}},
	{{12,  7}, {12, 26}, {39, 14}, {22, 41}}, //35
	{{ 6, 14}, { 6, 34}, {46, 10}, { 2, 64}},
	{{17,  4}, {29, 14}, {49, 10}, {24, 46}},
	{{ 4, 18}, {13, 32}, {48, 14}, {42, 32}},
	{{20,  4}, {40,  7}, {43, 22}, {10, 67}},
	{{19,  6}, {18, 31}, {34, 34}, {20, 61}},//40
};

int *QRspec_getEccBlockNum(int version, QRenc_ErrorCorrectionLevel level)
{
	int b1, b2;
	int data, ecc;
	int *array;

	b1 = eccTable[version][level][0];
	b2 = eccTable[version][level][1];

	data = QRspec_getMaximumCodeLength(version, level);
	ecc  = QRspec_getECCLength(version, level);

	array = (int *)malloc(sizeof(int) * 6);

	if(b2 == 0) {
		array[0] = b1;
		array[1] = data / b1;
		array[2] = ecc / b1;
		array[3] = array[4] = array[5] = 0;
	} else {
		array[0] = b1;
		array[1] = data / (b1 + b2);
		array[2] = ecc  / (b1 + b2);
		array[3] = b2;
		array[4] = array[1] + 1;
		array[5] = (ecc - (array[2] * b1)) / b2;
	}

	return array;
}
