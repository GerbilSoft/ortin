/***************************************************************************
 * Ortin (IS-NITRO management) (libortin)                                  *
 * ISNitro.cpp: IS-NITRO class.                                            *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "ISNitro.hpp"

// C includes.
#include <unistd.h>	// FIXME: usleep() for Windows

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <cstdio>

// C++ includes.
#include <algorithm>
#include <memory>
using std::unique_ptr;

#include "byteswap.h"

// Debug ROM
#include "bins/debugger_code.h"

/**
 * Initialize an IS-NITRO unit.
 * TODO: Enumerate IS-NITRO units and allow the user to select one.
 *
 * libusb context init/exit must be managed by the caller.
 *
 * @param ctx libusb_context. (nullptr for default)
 */
ISNitro::ISNitro(libusb_context *ctx)
	: m_ctx(ctx)
{
	// Open an IS-NITRO device.
	// TODO: This ID is for the IS-NITRO USG model.
	// Add more IDs for IS-NITRO NTR and IS-TWL?
	// TODO: Support for multiple IS-NITRO units.
	m_device = libusb_open_device_with_vid_pid(ctx, 0x0F6E, 0x0404);
	if (!m_device) {
		return;
	}

	// TODO: Error checking.

	// Set the active configuration.
	int ret = libusb_set_configuration(m_device, 1);
	if (ret < 0) {
		// Unable to set the device configuration.
		libusb_close(m_device);
		m_device = nullptr;
		return;
	}

	// Reset may be needed to avoid timeout errors.
	ret = libusb_reset_device(m_device);
	if (ret < 0) {
		// Unable to reset the device.
		libusb_close(m_device);
		m_device = nullptr;
		return;
	}

	// Claim the interface.
	ret = libusb_claim_interface(m_device, 0);
	if (ret < 0) {
		// Unable to claim the interface.
		libusb_close(m_device);
		m_device = nullptr;
		return;
	}
}

ISNitro::~ISNitro()
{
	if (m_device) {
		libusb_release_interface(m_device, 0);
		libusb_close(m_device);
	}
}

/**
 * Send a READ command.
 * @param cmd		[in] Command.
 * @param _slot		[in] Slot number for EMULATOR memory.
 * @param address	[in] Source address.
 * @param data		[out] Data.
 * @param len		[in] Length of data.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::sendReadCommand(uint16_t cmd, uint8_t _slot, uint32_t address, uint8_t *data, uint32_t len)
{
	NitroUSBCmd cdb;
	cdb.cmd = cpu_to_le16(cmd);
	cdb.op = NITRO_OP_READ;
	cdb._slot = _slot;
	cdb.address = cpu_to_le32(address);
	cdb.length = cpu_to_le32(len);
	cdb.zero = 0;

	// Send the READ command.
	int transferred = 0;
	int ret = libusb_bulk_transfer(m_device, BULK_EP_OUT,
		(uint8_t*)&cdb, (int)sizeof(cdb), &transferred, 1000);
	if (ret < 0) {
		printf("A ERR!!!!\n");
		return ret;
	}
	if (transferred != (int)sizeof(cdb)) {
		// Short write.
		printf("SHORT WRITE: transferred == %d, len == %d\n", transferred, (int)sizeof(cdb));
		return LIBUSB_ERROR_TIMEOUT;
	}

	// Read the data.
	ret = libusb_bulk_transfer(m_device, BULK_EP_IN,
		data, (int)len, &transferred, 1000);
	if (ret < 0) {
		printf("B ERR!!!!\n");
		printf("transferred == %d, len == %d\n", transferred, (int)len);
		return ret;
	}
	if (transferred != (int)len) {
		// Short read.
		printf("SHORT READ: transferred == %d, len == %d\n", transferred, (int)len);
		return LIBUSB_ERROR_TIMEOUT;
	}

	return 0;
}

/**
 * Send a WRITE command.
 * @param cmd		[in] Command.
 * @param _slot		[in] Slot number for EMULATOR memory.
 * @param address	[in] Destination address.
 * @param data		[in] Data.
 * @param len		[in] Length of data.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::sendWriteCommand(uint16_t cmd, uint8_t _slot, uint32_t address, const uint8_t *data, uint32_t len)
{
	if (len == 0) {
		// No payload. Send the command buffer directly.
		NitroUSBCmd cdb;
		cdb.cmd = cpu_to_le16(cmd);
		cdb.op = NITRO_OP_WRITE;
		cdb._slot = _slot;
		cdb.address = cpu_to_le32(address);
		cdb.length = 0;
		cdb.zero = 0;

		int transferred = 0;
		int ret = libusb_bulk_transfer(m_device, BULK_EP_OUT,
			(uint8_t*)&cdb, (int)sizeof(cdb), &transferred, 1000);
		if (ret < 0)
			return ret;
		if (transferred != (int)sizeof(cdb)) {
			// Short write.
			return LIBUSB_ERROR_TIMEOUT;
		}
		return 0;
	}

	// NOTE: We need to include the command header before the payload,
	// so we'll send 8 KB chunks to avoid memory issues.
	static const uint32_t CHUNK_SIZE = 1048576U;
	unique_ptr<uint8_t[]> cdb_raw(new uint8_t[sizeof(NitroUSBCmd)+CHUNK_SIZE]);
	NitroUSBCmd *const pCdb = reinterpret_cast<NitroUSBCmd*>(cdb_raw.get());
	uint8_t *const pData = &cdb_raw[sizeof(NitroUSBCmd)];

	pCdb->cmd = cpu_to_le16(cmd);
	pCdb->op = NITRO_OP_WRITE;
	pCdb->_slot = _slot;
	pCdb->zero = 0;

	while (len > 0) {
		const uint32_t curlen = std::min(len, CHUNK_SIZE);
		const uint32_t txlen = curlen + sizeof(NitroUSBCmd);
		pCdb->address = cpu_to_le32(address);
		pCdb->length = cpu_to_le32(curlen);
		memcpy(pData, data, curlen);

		int transferred = 0;
		int ret = libusb_bulk_transfer(m_device, BULK_EP_OUT,
			cdb_raw.get(), (int)txlen, &transferred, 1000);
		if (ret < 0)
			return ret;
		if (transferred != (int)txlen) {
			// Short write.
			return LIBUSB_ERROR_TIMEOUT;
		}

		address += curlen;
		data += curlen;
		len -= curlen;
	}

	return 0;
}

/**
 * Reset the entire IS-NITRO system.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::fullReset(void)
{
	// Turn off Slot 2 if it's enabled.
	// (Full Reset doesn't turn it off for some reason.)
	int ret = setSlotPower(2, false);
	if (ret < 0)
		return ret;

	static const uint8_t data[] = {NITRO_CMD_FULL_RESET, 0xF2};
	return sendWriteCommand(NITRO_CMD_FULL_RESET, 0, 0, data, sizeof(data));
}

/**
 * Set the RESET state of the Nintendo DS subsystem.
 *
 * Note that the LCDs will start to "fade" when in RESET.
 *
 * @param reset True to RESET; false to remove from RESET.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::ndsReset(bool reset)
{
	const uint8_t data[] = {NITRO_CMD_NDS_RESET, 0x00, reset, 0x00};
	return sendWriteCommand(NITRO_CMD_NDS_RESET, 0, 0, data, sizeof(data));
}

 /**
 * Set slot power.
 * @param _slot Slot number. (1 for DS, 2 for GBA)
 * @param on True to turn on; false to turn off.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::setSlotPower(uint8_t _slot, bool on)
{
	assert(_slot == 1 || _slot == 2);

	uint8_t data[] = {
		NITRO_CMD_SLOT_POWER, 0x00, 0x00, 0x00,
		0 /* device */, 0x00, 0x00, 0x00,
		on,   0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};

	int ret = 0;
	if (_slot == 1) {
		data[4] = 0x0A;	// slot 1
		ret = sendWriteCommand(NITRO_CMD_SLOT_POWER, 0, 0, data, sizeof(data));
	} else /*if (_slot == 2)*/ {
		data[4] = 0x02;	// slot 2 (primary?)
		ret = sendWriteCommand(NITRO_CMD_SLOT_POWER, 0, 0, data, sizeof(data));
		if (ret < 0)
			return ret;
		if (on) {
			data[4] = 0x04;	// slot 2 (secondary?)
			data[8] = 0;
			ret = sendWriteCommand(NITRO_CMD_SLOT_POWER, 0, 0, data, sizeof(data));
		}
	}

	return ret;
}

/**
 * Write to Slot-1 EMULATOR memory.
 *
 * NOTE: Caller should call this function in chunks itself for
 * better UI interactivity.
 *
 * @param _slot Emulated slot number. (1 for DS, 2 for GBA)
 * @param address Destination address.
 * @param data Data.
 * @param len Length of data.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::writeEmulationMemory(uint8_t _slot, uint32_t address, const uint8_t *data, uint32_t len)
{
	// NOTE: Must be a multiple of two bytes.
	assert(_slot == 1 || _slot == 2);
	assert(len % 2 == 0);
	return sendWriteCommand(NITRO_CMD_EMULATOR_MEMORY, _slot, address, data, len);
}

/**
 * Install the debugger ROM.
 * This is required in order to load an NDS game successfully.
 *
 * @param toFirmware If true, boot to NDS firmware instead of the game.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::installDebuggerROM(bool toFirmware)
{
	// Debugger ROM is installed at 0xFF80000 in EMULATOR memory.
	writeEmulationMemory(1, 0xFF80000, debugger_code, sizeof(debugger_code));

	// Set the ISID in Slot 2.
	uint8_t isid[1024];
	memset(isid, 0, sizeof(isid));
	memset(isid, 0xFF, 0x90);
	memset(&isid[0xA0], 0xFF, 0x10);
	isid[0xF6] = 0xFF;
	isid[0xF7] = 0xFF;
	isid[0x100] = 'I';
	isid[0x101] = 'S';
	isid[0x102] = 'I';
	isid[0x103] = 'D';
	isid[0x104] = 1;
	writeEmulationMemory(2, 0, isid, sizeof(isid));

	// Overwrite the debugging pointers in the ROM header.
	const uint32_t debug_ptrs[4] = {
		cpu_to_le32(0x8FF80000), cpu_to_le32((uint32_t)sizeof(debugger_code)),
		cpu_to_le32(0x02700000), cpu_to_le32(0x02700004),
	};
	if (!toFirmware) {
		writeEmulationMemory(1, 0x160, (const uint8_t*)debug_ptrs, sizeof(debug_ptrs));
	}
	return 0;
}

/**
 * Wait for the debugger ROM to initialize.
 * Debugger ROM must be installed and NDS must be out of reset.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::waitForDebuggerROM(void)
{
	uint8_t cmdSetCPU[] = {NITRO_CMD_SET_CPU, 0, 0, 0};

	// Try up to 1000 times.
	for (int i = 0; i < 1000; i++) {
		uint8_t bufARM9[8], bufARM7[8];

		// Set the current CPU to ARM9.
		cmdSetCPU[2] = NITRO_CPU_ARM9;
		int ret = sendWriteCommand(NITRO_CMD_SET_CPU, 0, 0, cmdSetCPU, sizeof(cmdSetCPU));
		if (ret < 0)
			return ret;

		// Read the debugger state. (cmd139?)
		ret = sendReadCommand(139, 0, 0, bufARM9, sizeof(bufARM9));
		if (ret < 0)
			return ret;

		// Set the current CPU to ARM7.
		cmdSetCPU[2] = NITRO_CPU_ARM7;
		ret = sendWriteCommand(NITRO_CMD_SET_CPU, 0, 0, cmdSetCPU, sizeof(cmdSetCPU));
		if (ret < 0)
			return ret;

		// Read the debugger state. (cmd139?)
		ret = sendReadCommand(139, 0, 0, bufARM7, sizeof(bufARM7));
		if (ret < 0)
			return ret;

		// Is the debugger initialized?
		if (bufARM9[3] == 1 && bufARM7[3] == 1) {
			// Debugger initialized!
			return 0;
		}

		// Wait 10ms and try again.
		usleep(10000);
	}

	// Debugger ROM failed to initialize...
	return LIBUSB_ERROR_TIMEOUT;
}

/**
 * Write to the NEC CPU's memory.
 *
 * @param address Destination address.
 * @param data Data.
 * @param len Length of data.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::writeNECMemory(uint32_t address, const uint8_t *data, uint32_t len)
{
	// NEC commands have an 8-byte structure, followed by the payload.
	// Payload must be a multiple of 2 bytes.
	assert(len % 2 == 0);
	const uint32_t cdblen = len + sizeof(NitroNECCommand);
	unique_ptr<uint8_t[]> cdb(new uint8_t[cdblen]);
	NitroNECCommand *const pNecCmd = reinterpret_cast<NitroNECCommand*>(cdb.get());
	pNecCmd->cmd = NITRO_CMD_NEC_MEMORY;
	pNecCmd->unitSize = 2;
	pNecCmd->length = cpu_to_le16(len / 2);
	pNecCmd->address = cpu_to_le32(address);
	memcpy(&cdb[sizeof(NitroNECCommand)], data, len);
	return sendWriteCommand(NITRO_CMD_NEC_MEMORY, 0, 0, cdb.get(), cdblen);
}

/**
 * Unlock the AV functionality.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::unlockAV(void)
{
	static const uint8_t cmd1[] = {0x59, 0x00};
	int ret = writeNECMemory(0x8000010, cmd1, sizeof(cmd1));
	if (ret < 0)
		return ret;

	static const uint8_t cmd2[] = {0x4F, 0x00};
	ret = writeNECMemory(0x8000012, cmd2, sizeof(cmd2));
	if (ret < 0)
		return ret;

	static const uint8_t cmd3[] = {0x4B, 0x00};
	ret = writeNECMemory(0x8000014, cmd3, sizeof(cmd3));
	if (ret < 0)
		return ret;

	static const uint8_t cmd4[] = {0x4F, 0x00};
	ret = writeNECMemory(0x8000016, cmd4, sizeof(cmd4));
	return ret;
}

/**
 * Write a monitor configuration register.
 * @param reg Register number.
 * @param value Value.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::writeMonitorConfigRegister(uint8_t reg, uint16_t value)
{
	const uint8_t cmd1[] = {reg, 0};
	int ret = writeNECMemory(0x8000030, cmd1, sizeof(cmd1));
	if (ret < 0)
		return ret;

	const uint8_t cmd2[] = {(uint8_t)(value & 0xFF), 0};
	ret = writeNECMemory(0x8000034, cmd2, sizeof(cmd2));
	if (ret < 0)
		return ret;

	const uint8_t cmd3[] = {(uint8_t)(value >> 8), 0};
	return writeNECMemory(0x8000036, cmd3, sizeof(cmd3));
}

/**
 * Set the background color.
 * @param bg_color Background color. (ARGB32)
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::setBgColor(uint32_t bg_color)
{
	uint8_t cmd[] = {(uint8_t)(bg_color & 0xFF), 0x00};
	int ret = writeNECMemory(0x800001C, cmd, sizeof(cmd));
	if (ret < 0)
		return ret;
	bg_color >>= 8;
	cmd[0] = (uint8_t)(bg_color & 0xFF);
	ret = writeNECMemory(0x800001A, cmd, sizeof(cmd));
	if (ret < 0)
		return ret;
	bg_color >>= 8;
	cmd[0] = (uint8_t)(bg_color & 0xFF);
	return writeNECMemory(0x8000018, cmd, sizeof(cmd));
}

/**
 * Set the AV mode settings.
 * @param mode AV mode.
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::setAVModeSettings(const NitroAVModeSettings_t *mode)
{
	// TODO: Change interlaced to bitfields; add rotation.
	// Unlock the AV functionality.
	int ret = unlockAV();
	if (ret < 0)
		return ret;

	// AV1 monitor parameters
	ret = writeMonitorConfigRegister(0x80, (mode->av[0].aspect_ratio ? 192 : 225));
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x81, 352);
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x82, 44);
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x83, (44 - (mode->av[0].spacing / 2)));
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x84, mode->av[0].spacing);
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x85, !!mode->av[0].interlaced);
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x86, !!mode->av[0].aspect_ratio);
	if (ret < 0)
		return ret;

	// AV2 monitor parameters
	ret = writeMonitorConfigRegister(0x00, (mode->av[1].aspect_ratio ? 192 : 225));
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x01, 352);
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x02, 44);
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x03, (44 - (mode->av[1].spacing / 2)));
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x04, mode->av[1].spacing);
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x05, !!mode->av[1].interlaced);
	if (ret < 0)
		return ret;
	ret = writeMonitorConfigRegister(0x06, !!mode->av[1].aspect_ratio);
	if (ret < 0)
		return ret;

	// Set the background color.
	ret = setBgColor(mode->bg_color);
	if (ret < 0)
		return ret;

	// Monitor state bitfield.
	uint8_t monitor_state =  (uint8_t)mode->av[1].mode |
				((uint8_t)mode->rotation << 2) |
				((uint8_t)mode->av[0].mode << 4) |
				((uint8_t)mode->deflicker << 6);
	const uint8_t cmd[] = {monitor_state, 0};
	ret = writeNECMemory(0x800001E, cmd, sizeof(cmd));
	if (ret < 0)
		return ret;

	// Disable the cursor.
	// TODO: Separate into a separate function so we can make use of it later?
	// X,Y pos are set to 255 to hide the cursor.
	static const uint8_t cmd_cursorX[] = {0xFF, 0x00};
	ret = writeNECMemory(0x800002C, cmd_cursorX, sizeof(cmd_cursorX));
	if (ret < 0)
		return ret;
	static const uint8_t cmd_cursorY[] = {0xFF, 0x00};
	return writeNECMemory(0x800002E, cmd_cursorY, sizeof(cmd_cursorX));
}

/**
 * Insert a breakpoint into a CPU to pause it.
 * CPU must be in BREAK in order to read from its memory space.
 * @param cpu CPU index. (See NitroCPU_e.)
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::breakProcessor(uint8_t cpu)
{
	// TODO: Split operations into separate functions?
	assert(cpu == 0 || cpu == 1);

	// Set the current CPU.
	const uint8_t cmdSetCPU[] = {NITRO_CMD_SET_CPU, 0, cpu, 0};
	int ret = sendWriteCommand(NITRO_CMD_SET_CPU, 0, 0, cmdSetCPU, sizeof(cmdSetCPU));
	if (ret < 0)
		return ret;

	// Toggle the FIQ pin for the CPU.
	ret = sendWriteCommand(NITRO_CMD_SET_FIQ_PIN, 0, 1, nullptr, 0);
	if (ret < 0)
		return ret;
	ret = sendWriteCommand(NITRO_CMD_SET_FIQ_PIN, 0, 0, nullptr, 0);
	if (ret < 0)
		return ret;

	// Do "something" with A0 for the CPU...
	const uint8_t cmdDoSomethingA0[] = {NITRO_CMD_DO_SOMETHING_A0, cpu};
	ret = sendWriteCommand(NITRO_CMD_DO_SOMETHING_A0, 0, 0, cmdDoSomethingA0, sizeof(cmdDoSomethingA0));
	if (ret < 0)
		return ret;

	// Set breakpoints.
	// TODO: Breakpoint builder.
	// 8 == begin break
	const uint32_t cmdBkpt[] = {cpu_to_le32(NITRO_CMD_SET_BREAKPOINTS), cpu_to_le32(4), cpu_to_le32(8)};
	ret = sendWriteCommand(NITRO_CMD_SET_BREAKPOINTS, 0, 0, (const uint8_t*)cmdBkpt, sizeof(cmdBkpt));
	if (ret < 0)
		return ret;

	return ret;
}

/**
 * Continue the CPU from break.
 * @param cpu CPU index. (See NitroCPU_e.)
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::continueProcessor(uint8_t cpu)
{
	// TODO: Split operations into separate functions?
	assert(cpu == 0 || cpu == 1);

	// Set the current CPU.
	const uint8_t cmdSetCPU[] = {NITRO_CMD_SET_CPU, 0, cpu, 0};
	int ret = sendWriteCommand(NITRO_CMD_SET_CPU, 0, 0, cmdSetCPU, sizeof(cmdSetCPU));
	if (ret < 0)
		return ret;

	// cmd 135?
	static const uint8_t cmd135[] = {135, 0, 2, 0,    0,0,0,0, 0,0,0,0};
	ret = sendWriteCommand(135, 0, 0, cmd135, sizeof(cmd135));
	if (ret < 0)
		return ret;

	// Set breakpoints.
	// TODO: Breakpoint builder.
	// 9 == continue from break
	const uint32_t cmdBkpt[] = {cpu_to_le32(NITRO_CMD_SET_BREAKPOINTS), cpu_to_le32(4), cpu_to_le32(9)};
	ret = sendWriteCommand(NITRO_CMD_SET_BREAKPOINTS, 0, 0, (const uint8_t*)cmdBkpt, sizeof(cmdBkpt));
	if (ret < 0)
		return ret;

	// cmd 133?
	static const uint8_t cmd133[] = {133, 0};
	ret = sendWriteCommand(133, 0, 0, cmd133, sizeof(cmd133));
	if (ret < 0)
		return ret;

	return ret;
}

/**
 * Send cmd174 to the specified CPU.
 * This is usually done after initializing the debugger ROM.
 * @param cpu CPU index. (See NitroCPU_e.)
 * @return 0 on success; libusb error code on error.
 */
int ISNitro::sendCpuCMD174(uint8_t cpu)
{
	// TODO: Split operations into separate functions?
	assert(cpu == 0 || cpu == 1);

	// Set the current CPU.
	const uint8_t cmdSetCPU[] = {NITRO_CMD_SET_CPU, 0, cpu, 0};
	int ret = sendWriteCommand(NITRO_CMD_SET_CPU, 0, 0, cmdSetCPU, sizeof(cmdSetCPU));
	if (ret < 0)
		return ret;

	// Send cmd174.
	const uint32_t cmd174[] = {cpu_to_le32(174), cpu_to_le32(3), cpu_to_le32(1), 0, 0};
	return sendWriteCommand(174, 0, 0, (const uint8_t*)cmd174, sizeof(cmd174));
}
