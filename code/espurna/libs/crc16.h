// -----------------------------------------------------------------------------
// Helper function to calculate arbitrary crc16 over arbitrarily sized C arrays
// ref: https://en.wikipedia.org/wiki/Cyclic_redundancy_check
// ref: https://sudonull.com/post/8849-CRC16-Checksum-Reliability
// ref: https://habr.com/ru/post/428746/
// -----------------------------------------------------------------------------

#pragma once

#pragma GCC optimize ("O2")

// Reverse the top and bottom nibble then swap them.
inline uint8_t reverse_bits(uint8_t byte) {
    static unsigned char reverse_table[16] {
        0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
        0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF
    };
    return (reverse_table[byte & 0b1111] << 4) | reverse_table[byte >> 4];
}

inline uint16_t reverse_word(uint16_t word) {
  return ((reverse_bits(word & 0xFF) << 8) | reverse_bits(word >> 8));
}

inline uint16_t crc16_common(uint8_t *data, uint8_t len, uint16_t poly, uint16_t init, uint16_t doXor, bool refIn, bool refOut) {
    uint16_t crc = init;

    while (len--) {
        if (refIn) {
            crc = ((uint16_t)reverse_bits(*data++) << 8) ^ crc;
        } else {
            crc = ((uint16_t)*data++ << 8) ^ crc;
        }

        for (int y = 0; y < 8; y++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ poly;
            } else {
                crc = crc << 1;
            }
        }
    }

    if (refOut) {
        crc = reverse_word(crc);
    }

    return (crc ^ doXor);
}

#pragma GCC optimize ("Os")
