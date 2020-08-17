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
	NITRO_CMD_SET_CPU		= 0x8B,	// Set current CPU for operations (0 == ARM9, 1 == ARM7)
	NITRO_CMD_SET_FIQ_PIN		= 0xAA,	// Set FIQ pin state for the current CPU
	NITRO_CMD_SLOT_POWER		= 0xAD,
	NITRO_CMD_SET_BREAKPOINTS	= 0xBD,	// Set breakpoints
} NitroCommand_e;

/**
 * AV port mode
 */
typedef enum {
	NITRO_AV_MODE_OFF	= 0,
	NITRO_AV_MODE_UPPER	= 1,
	NITRO_AV_MODE_LOWER	= 2,
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
	uint8_t deflicker;		// Deflicker (see NitroAVDeflicker_e)
	uint8_t rotation;		// Rotation (see NitroAVRotation_e)
} NitroAVModeSettings_t;

/**
 * AV mode: Deflicker
 */
typedef enum {
	NITRO_AV_DEFLICKER_DISABLED	= 0,
	NITRO_AV_DEFLICKER_NORMAL	= 1,
	NITRO_AV_DEFLICKER_ALTERNATE	= 3,
} NitroAVDeflicker_e;

/**
 * AV mode: Rotation
 */
typedef enum {
	NITRO_AV_ROTATION_NONE		= 0,
	NITRO_AV_ROTATION_LEFT		= 1,
	NITRO_AV_ROTATION_RIGHT		= 3,
} NitroAVRotation_e;

/**
 * NEC memory write command.
 */
typedef struct _NitroNECCommand {
	uint8_t cmd;		// Command (see NitroCommand_e)
	uint8_t unitSize;
	uint16_t length;	// Data length
	uint32_t address;	// Destination address
} NitroNECCommand;

/**
 * NEC registers.
 */
typedef enum {
	NITRO_NEC_NDS_REG0			= 0x08000000,	// See NitroNECNDSReg0_e.
	NITRO_NEC_NDS_REG1			= 0x08000002,	// See NitroNECNDSReg1_e.

	NITRO_NEC_REG_FORWARD0			= 0x08000004,
	NITRO_NEC_REG_FORWARD1			= 0x08000006,
	NITRO_NEC_REG_FORWARD_EN		= 0x08000008,
	NITRO_NEC_REG_FORWARD_CFG		= 0x0800000A,
	NITRO_NEC_REG_FRAME0			= 0x0800000C,
	NITRO_NEC_REG_FRAME1			= 0x0800000E,

	NITRO_NEC_REG_VIDEO_UNLOCK0		= 0x08000010,
	NITRO_NEC_REG_VIDEO_UNLOCK1		= 0x08000012,
	NITRO_NEC_REG_VIDEO_UNLOCK2		= 0x08000014,
	NITRO_NEC_REG_VIDEO_UNLOCK3		= 0x08000016,

	NITRO_NEC_REG_MONITOR_BG_R		= 0x08000018,
	NITRO_NEC_REG_MONITOR_BG_G		= 0x0800001A,
	NITRO_NEC_REG_MONITOR_BG_B		= 0x0800001C,

	NITRO_NEC_REG_MONITOR_STATE		= 0x0800001E,

	NITRO_NEC_REG_CURSOR_IMAGE_OFFSET	= 0x08000022,
	NITRO_NEC_REG_CURSOR_PIXEL_LO		= 0x08000024,
	NITRO_NEC_REG_CURSOR_PIXEL_HI		= 0x08000026,

	NITRO_NEC_REG_COUNTER_LO		= 0x08000028,
	NITRO_NEC_REG_COUNTER_HI		= 0x0800002A,

	NITRO_NEC_REG_CURSOR_POS_X		= 0x0800002C,
	NITRO_NEC_REG_CURSOR_POS_Y		= 0x0800002E,

	NITRO_NEC_REG_MONITOR_SEL		= 0x08000030,
	NITRO_NEC_REG_MONITOR_DATA_LO		= 0x08000034,
	NITRO_NEC_REG_MONITOR_DATA_HI		= 0x08000036,
} NitroNECVideoRegister_e;

/**
 * NEC NDS register 0 bits.
 */
typedef enum {
	NITRO_NEC_NDS_REG0_DEBUG_BUTTON		= (1U << 0),
	NITRO_NEC_NDS_REG0_COVER		= (1U << 1),
	NITRO_NEC_NDS_REG0_RESET		= (1U << 4),
} NitroNECNDSReg0_e;

/**
 * NEC NDS register 0 bits.
 */
typedef enum {
	NITRO_NEC_NDS_REG1_WRITE_PROTECTION	= (1U << 0),
	NITRO_NEC_NDS_REG1_BOOT_COMPLETE	= (1U << 1),
	NITRO_NEC_NDS_REG1_POWER		= (1U << 4),
} NitroNECNDSReg1_e;
/**
 * CPU index.
 */
typedef enum {
	NITRO_CPU_ARM9	= 0,
	NITRO_CPU_ARM7	= 1,
} NitroCPU_e;

#ifdef __cplusplus
}
#endif

#endif /* __ORTIN_NITRO_USB_CMDS__ */
