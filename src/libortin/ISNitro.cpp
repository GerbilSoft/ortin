/***************************************************************************
 * Ortin (IS-NITRO management) (libortin)                                  *
 * ISNitro.cpp: IS-NITRO class.                                            *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "ISNitro.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <memory>
using std::unique_ptr;

#include "byteswap.h"
#include "nitro-usb-cmds.h"

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
 * Send a WRITE command.
 * @param cmd Command.
 * @param unk Unknown.
 * @param _slot Slot number for EMULATOR memory.
 * @param address Destination address.
 * @param data Data.
 * @param len Length of data.
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
	// TODO: Slot 2.
	assert(_slot == 1);

	const uint8_t data[] = {NITRO_CMD_SLOT_POWER, 0x00, 0x00, 0x00,
		0x0a, 0x00, 0x00, 0x00,
		on,   0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	return sendWriteCommand(NITRO_CMD_SLOT_POWER, 0, 0, data, sizeof(data));
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
