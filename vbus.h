// vbus.h
//
// (c) Hewell Technology Ltd. 2014
//
//****************************************************************************

#pragma once

typedef struct __attribute__((packed)) _VBUS_HEADER
{
	unsigned char syncByte;
	unsigned short dest;
	unsigned short source;
	unsigned char ver;
} VBUS_HEADER, *PVBUS_HEADER;

typedef struct _FRAME_STRUCT
{
	unsigned char bytes[4];
	unsigned char septett;
	unsigned char crc;
} FRAME_STRUCT, *PFRAME_STRUCT;

typedef struct _VBUS_V1_CMD
{
	VBUS_HEADER h;
	unsigned short cmd;
	unsigned char frameCnt;
	unsigned char crc;
	FRAME_STRUCT frame[];
} VBUS_V1_CMD, *PVBUS_V1_CMD;

extern unsigned char vbus_calc_crc(const unsigned char *buffer, int offset, int length);
extern void vbus_extract_septett(unsigned char *buffer, int offset, int length);
extern void vbus_inject_septett(unsigned char *buffer, int offset, int length);
