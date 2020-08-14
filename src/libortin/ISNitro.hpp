/***************************************************************************
 * Ortin (IS-NITRO management) (libortin)                                  *
 * ISNitro.hpp: IS-NITRO class.                                            *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ORTIN_ISNITRO_HPP__
#define __ORTIN_ISNITRO_HPP__

#include <stdint.h>
#include <libusb.h>

#include "nitro-usb-cmds.h"

class ISNitro
{
	public:
		/**
		 * Initialize an IS-NITRO unit.
		 * TODO: Enumerate IS-NITRO units and allow the user to select one.
		 *
		 * libusb context init/exit must be managed by the caller.
		 *
		 * @param ctx libusb_context. (nullptr for default)
		 */
		ISNitro(libusb_context *ctx = nullptr);

		~ISNitro();

	private:
		ISNitro(const ISNitro &);
		ISNitro &operator=(const ISNitro&);

	public:
		static const uint8_t BULK_EP_OUT	= 0x01;
		static const uint8_t BULK_EP_IN		= 0x82;
		// TODO: What is this endpoint used for?
		static const uint8_t BULK_EP_IN_3	= 0x83;

	public:
		inline bool isOpen(void) const
		{
			return (m_device != nullptr);
		}

	protected:
		/**
		 * Send a WRITE command.
		 * @param cmd Command.
		 * @param _slot Slot number for EMULATOR memory.
		 * @param address Destination address.
		 * @param data Data.
		 * @param len Length of data.
		 * @return 0 on success; libusb error code on error.
		 */
		int sendWriteCommand(uint16_t cmd, uint8_t _slot, uint32_t address, const uint8_t *data, uint32_t len);

	public:
		/**
		 * Reset the entire IS-NITRO system.
		 * @return 0 on success; libusb error code on error.
		 */
		int fullReset(void);

		/**
		 * Set the RESET state of the Nintendo DS subsystem.
		 *
		 * Note that the LCDs will start to "fade" when in RESET.
		 *
		 * @param reset True to RESET; false to remove from RESET.
		 * @return 0 on success; libusb error code on error.
		 */
		int ndsReset(bool reset);

		/**
		 * Set slot power.
		 * @param _slot Slot number. (1 for DS, 2 for GBA)
		 * @param on True to turn on; false to turn off.
		 * @return 0 on success; libusb error code on error.
		 */
		int setSlotPower(uint8_t _slot, bool on);

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
		int writeEmulationMemory(uint8_t _slot, uint32_t address, const uint8_t *data, uint32_t len);

		/**
		 * Write to the NEC CPU's memory.
		 *
		 * @param address Destination address.
		 * @param data Data.
		 * @param len Length of data.
		 * @return 0 on success; libusb error code on error.
		 */
		int writeNECMemory(uint32_t address, const uint8_t *data, uint32_t len);

		/**
		 * Unlock the AV functionality.
		 * @return 0 on success; libusb error code on error.
		 */
		int unlockAV(void);

		/**
		 * Set an AV port's output mode.
		 * @param av1mode AV1 mode.
		 * @param av2mode AV2 mode.
		 * @param av1interlaced True for AV1 interlaced; false for AV1 non-interlaced.
		 * @param av2interlaced True for AV1 interlaced; false for AV1 non-interlaced.
		 * @return 0 on success; libusb error code on error.
		 */
		int setAVMode(NitroAVMode_e av1mode, NitroAVMode_e av2mode, bool av1interlaced, bool av2interlaced);

	protected:
		libusb_context *m_ctx;
		libusb_device_handle *m_device;
};

#endif /* __ORTIN_ISNITRO_HPP__ */
