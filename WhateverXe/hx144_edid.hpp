#pragma once
// HX144: opt-in, EDID-only second DTD for the verified AUO B156HAN15.K.
// No link register, lane, panel power, clock, backlight or mode-select writes.
// Keep the 60-Hz preferred DTD to retain the existing known-good boot mode.
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace HX144EDID {
inline bool injectSecondary144(uint8_t *base, size_t length, uint32_t ddi) {
    if (!base || length != 128 || ddi != 0) return false;
    static const uint8_t identity[12] = {
        0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x06,0xAF,0xAD,0x7E
    };
    static const uint8_t dtd60[18] = {
        0xA6,0x37,0x80,0xB7,0x70,0x38,0x31,0x40,0x30,
        0x20,0xA5,0x00,0x58,0xC1,0x10,0x00,0x00,0x18
    };
    static const uint8_t rangeTag[5] = {0x00,0x00,0x00,0xFD,0x00};
    static const uint8_t vendorTag[5] = {0x00,0x00,0x00,0xFE,0x00};
    static const uint8_t modelText[13] = {
        'B','1','5','6','H','A','N','1','5','.','K',' ',0x0A
    };
    if (memcmp(base,identity,sizeof(identity)) != 0 ||
        memcmp(base+54,dtd60,sizeof(dtd60)) != 0 ||
        memcmp(base+72,rangeTag,sizeof(rangeTag)) != 0 ||
        memcmp(base+90,vendorTag,sizeof(vendorTag)) != 0 ||
        memcmp(base+108,vendorTag,sizeof(vendorTag)) != 0 ||
        memcmp(base+113,modelText,sizeof(modelText)) != 0 ||
        base[81] != 0x22 || // original max pixel clock is 340 MHz
        base[126] != 1) return false;
    unsigned checksum=0;
    for (size_t i=0;i<128;i++) checksum+=base[i];
    if ((checksum & 255U)!=0) return false;
    // Add the second DTD in place of the unneeded manufacturer text.
    // Clock in base EDID units of 10 kHz: 34190 = 341.900 MHz.
    // The panel's own DisplayID Type VII advertises this timing.
    // Keep first DTD at 142.460 MHz; max advertised range is adjusted below.
    memcpy(base+90,dtd60,sizeof(dtd60));
    base[90]=0x8E;
    base[91]=0x85;
    // DisplayID says 341.900 MHz, while the coarse 10-MHz range field
    // is 0x22 (340 MHz). This may be legitimate nearest-10-MHz rounding.
    // For this opt-in override only, advertise 350 MHz to avoid a strict
    // parser treating the rounded 340 as a hard ceiling for the new DTD.
    // This affects only the in-memory EDID, not the hardware pixel clock.
    base[81]=0x23;
    checksum=0;
    for (size_t i=0;i<127;i++) checksum+=base[i];
    base[127]=static_cast<uint8_t>(0U-checksum);
    return true;
}
} // namespace HX144EDID
