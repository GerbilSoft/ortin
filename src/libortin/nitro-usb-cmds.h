/***************************************************************************
 * Ortin: libortin                                                         *
 * nitro-usb-cmds.h: IS-NITRO USB command definitions.                     *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ORTIN_NITRO_USB_CMDS__
#define __ORTIN_NITRO_USB_CMDS__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * USB commands.
 * All fields are little-endian.
 */
typedef struct _NitroUSBCmd {
	uint16_t cmd;		// Command (see NitroCommand_e)
	uint8_t op;		// Opcode (see NitroOpcode_e)
	uint8_t _slot;		// Slot number for EMULATOR memory.
	uint32_t address;	// Memory address
	uint32_t length;	// Data length
	uint32_t zero;		// Zero
} NitroUSBCmd;
//ASSERT_STRUCT(NitroUSBCmd, 16);

/**
 * Nitro USB commands: Opcodes.
 */
typedef enum {
	NITRO_OP_WRITE	= 0x10,
	NITRO_OP_READ	= 0x11,
} NitroOpcode_e;

/**
 * Nitro USB commands: Commands.
 */
typedef enum {
	NITRO_CMD_EMULATOR_MEMORY	= 0x00,
	NITRO_CMD_NEC_MEMORY		= 0x26,
	NITRO_CMD_FULL_RESET		= 0x81,
	NITRO_CMD_NDS_RESET		= 0x8A,
	NITRO_CMD_SLOT_POWER		= 0xAD,
} NitroCommand_e;

/**
 * AV port mode
 */
typedef enum {
	NITRO_AV_MODE_OFF	= 0,
	NITRO_AV_MODE_TOP	= 1,
	NITRO_AV_MODE_BOTTOM	= 2,
	NITRO_AV_MODE_BOTH	= 3,
} NitroAVMode_e;

/**
 * AV mode settings for a single output.
 * For internal use; does not match the actual registers!
 */
typedef struct _NitroAVModeScreen_t {
	NitroAVMode_e mode;	// Off, Top, Bottom, Both
	bool interlaced;	// True for interlaced
	bool aspect_ratio;	// True for correct aspect ratio
	int spacing;		// Spacing between screens (minimum 1)
} NitroAVModeScreen_t;

/**
 * AV mode settings for both outputs.
 * For internal use; does not match the actual registers!
 */
typedef struct _NitroAVModeSettings_t {
	NitroAVModeScreen_t av[2];	// Per-screen settings
	uint32_t bg_color;		// Background color (ARGB32 format)
	uint8_t rotation;		// Rotation (see NitroAVRotation_e)
	uint8_t deflicker;		// Deflicker (see NitroAVDeflicker_e)
} NitroAVModeSettings_t;

/**
 * AV mode: Rotation
 */
typedef enum {
	NITRO_AV_ROTATION_NONE		= 0,
	NITRO_AV_ROTATION_LEFT		= 1,
	NITRO_AV_ROTATION_UPSIDE_DOWN	= 2,	// ???
	NITRO_AV_ROTATION_RIGHT		= 3,
} NitroAVRotation_e;

/**
 * AV mode: Deflicker
 */
typedef enum {
	NITRO_AV_DEFLICKER_DISABLED	= 0,
	NITRO_AV_DEFLICKER_NORMAL	= 1,
	NITRO_AV_DEFLICKER_ALTERNATE	= 3,
} NitroAVDeflicker_e;

/**
 * NEC memory write command.
 */
typedef struct _NitroNECCommand {
	uint8_t cmd;		// Command (see NitroCommand_e)
	uint8_t unitSize;
	uint16_t length;	// Data length
	uint32_t address;	// Destination address
} NitroNECCommand;

#ifdef __cplusplus
}
#endif

#endif /* __ORTIN_NITRO_USB_CMDS__ */
