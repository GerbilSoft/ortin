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
		 * Send a READ command.
		 * @param cmd		[in] Command.
		 * @param _slot		[in] Slot number for EMULATOR memory.
		 * @param address	[in] Source address.
		 * @param data		[out] Data.
		 * @param len		[in] Length of data.
		 * @return 0 on success; libusb error code on error.
		 */
		int sendReadCommand(uint16_t cmd, uint8_t _slot, uint32_t address, uint8_t *data, uint32_t len);

		/**
		 * Send a WRITE command.
		 * @param cmd		[in] Command.
		 * @param _slot		[in] Slot number for EMULATOR memory.
		 * @param address	[in] Destination address.
		 * @param data		[in] Data.
		 * @param len		[in] Length of data.
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
		 * Install the debugger ROM.
		 * This is required in order to load an NDS game successfully.
		 *
		 * @param toFirmware If true, boot to NDS firmware instead of the game.
		 * @return 0 on success; libusb error code on error.
		 */
		int installDebuggerROM(bool toFirmware = false);

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
		 * Write a monitor configuration register.
		 * @param reg Register number.
		 * @param value Value.
		 * @return 0 on success; libusb error code on error.
		 */
		int writeMonitorConfigRegister(uint8_t reg, uint16_t value);

		/**
		 * Set the background color.
		 * @param bg_color Background color. (ARGB32)
		 * @return 0 on success; libusb error code on error.
		 */
		int setBgColor(uint32_t bg_color);

		/**
		 * Set the AV mode settings.
		 * @param mode AV mode settings.
		 * @return 0 on success; libusb error code on error.
		 */
		int setAVModeSettings(const NitroAVModeSettings_t *mode);

		/**
		 * Insert a breakpoint into a CPU to pause it.
		 * CPU must be in BREAK in order to read from its memory space.
		 * @param cpu CPU index. (0 == ARM9, 1 == ARM7)
		 * @return 0 on success; libusb error code on error.
		 */
		int breakProcessor(uint8_t cpu);

		/**
		 * Continue the CPU from break.
		 * @param cpu CPU index. (0 == ARM9, 1 == ARM7)
		 * @return 0 on success; libusb error code on error.
		 */
		int continueProcessor(uint8_t cpu);

	protected:
		libusb_context *m_ctx;
		libusb_device_handle *m_device;
};

#endif /* __ORTIN_ISNITRO_HPP__ */
