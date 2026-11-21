#ifndef TRACEKIT_USB_HANDBACK_H
#define TRACEKIT_USB_HANDBACK_H

// Gives the chip's internal USB PHY back to USB-Serial-JTAG before a software
// reset, after TinyUSB (the Serial link's CDC) had it.
//
// The PHY mux lives in the RTC domain, which a software reset keeps: without
// this, the next firmware -- the ROM download mode of `btp -bootloader`, the
// recovery of `btp -recovery` -- came up with the PHY still on the USB-OTG
// controller that nobody drives any more, and the board vanished from the USB
// until the cable was replugged. Switching the mux alone is not enough either:
// the host never sees the device leave, so it does not enumerate the
// USB-Serial-JTAG port that takes its place. D+ and D- are pulled low for a
// moment first, which the host reads as a disconnect.
//
// Safe to call when TinyUSB was never installed (the pads are then not the
// OTG's and only the mux bits are cleared). ESP32-S3 only; a no-op elsewhere.

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP32S3
#include <esp_rom_sys.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>
#include <soc/usb_wrap_struct.h>
#endif

namespace tracekit {

inline void usb_hand_back_to_jtag() noexcept {
#if CONFIG_IDF_TARGET_ESP32S3
    if (USB_WRAP.otg_conf.pad_enable) {
        // A disconnect the host can see: no pull-up on D+, both lines low.
        USB_WRAP.otg_conf.dp_pullup = 0;
        USB_WRAP.otg_conf.dm_pullup = 0;
        USB_WRAP.otg_conf.dp_pulldown = 1;
        USB_WRAP.otg_conf.dm_pulldown = 1;
        USB_WRAP.otg_conf.pad_pull_override = 1;
        // Hosts take a few ms to notice a detach; 100 ms is safe for all.
        esp_rom_delay_us(100U * 1000U);
        USB_WRAP.otg_conf.pad_pull_override = 0;
        USB_WRAP.otg_conf.pad_enable = 0;
    }
    // Hardware control of the mux: the internal PHY goes to USB-Serial-JTAG,
    // the port the ROM, the bootloader and the recovery talk through.
    CLEAR_PERI_REG_MASK(RTC_CNTL_USB_CONF_REG,
                        RTC_CNTL_SW_HW_USB_PHY_SEL | RTC_CNTL_SW_USB_PHY_SEL | RTC_CNTL_USB_PAD_ENABLE);
#endif
}

}  // namespace tracekit

#endif  // TRACEKIT_USB_HANDBACK_H
