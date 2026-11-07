/*

MATTER FACTORY DATA OF TUYA MATTER FIRMWARE (BK7231N, 2 MiB)

Tuya's Matter firmware keeps the factory identity of the board (DAC, its key, PAI, certification declaration,
passcode, SPAKE2+ verifier, VID / PID) in its encrypted key-value store at the end of the flash: a key block and 4 KiB
data blocks of 128-byte pages, AES-128-ECB (format as in bk7231tools.analysis.kvstorage). ESPurna never writes there,
so the identity stays on the chip after the stock firmware is gone. This reads it, read only.

No Arduino or SDK dependency: the flash is read through a callback, so the same code runs in a host test.

*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace espurna {
namespace beken {
namespace tuya_kv {

using FlashRead = bool (*)(uint32_t offset, uint8_t* out, size_t size);

// "chip-factory/..." entries of the store, converted to the formats the Matter platform layer reads
// (the Quectel factory table: DER certificates, raw P-256 keys, base64 salt / verifier)
struct MatterFactory {
    std::vector<uint8_t> dac;          // DER
    std::vector<uint8_t> pai;          // DER
    std::vector<uint8_t> declaration;  // DER (CMS)
    std::vector<uint8_t> dac_public;   // 65 bytes, uncompressed point
    std::vector<uint8_t> dac_private;  // 32 bytes
    std::string salt;                  // base64
    std::string verifier;              // base64 of w0 || L (97 bytes)
    std::string vendor_name;
    std::string product_name;
    std::string serial;
    uint32_t iterations { 0 };
    uint32_t passcode { 0 };
    uint16_t discriminator { 0 };
    uint16_t vendor_id { 0 };
    uint16_t product_id { 0 };
    uint16_t hardware_version { 0 };
    uint32_t offset { 0 };             // flash offset of the store
};

// Looks for the store in the last 128 KiB of a flash of flash_size bytes and reads the Matter factory data out of it.
// False when there is no store, or it holds no complete Matter identity
bool load_matter_factory(FlashRead read, uint32_t flash_size, MatterFactory& out);

// Helpers, public for the host test
bool aes128_ecb_decrypt(const uint8_t key[16], const uint8_t* in, uint8_t* out, size_t size);
bool base64_decode(const char* in, size_t size, std::vector<uint8_t>& out);
std::string base64_encode(const uint8_t* data, size_t size);

} // namespace tuya_kv
} // namespace beken
} // namespace espurna
