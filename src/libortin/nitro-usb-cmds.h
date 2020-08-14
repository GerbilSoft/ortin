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
	NITRO_CMD_FULL_RESET		= 0x81,
	NITRO_CMD_NDS_RESET		= 0x8A,
	NITRO_CMD_SLOT_POWER		= 0xAD,
} NitroCommand_e;

#ifdef __cplusplus
}
#endif

#endif /* __ORTIN_NITRO_USB_CMDS__ */
