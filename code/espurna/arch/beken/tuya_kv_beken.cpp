/*

MATTER FACTORY DATA OF TUYA MATTER FIRMWARE (ref. tuya_kv_beken.h)

Store layout (bk7231tools.analysis.kvstorage, the Tuya SDK's "simple flash"):
- key block, 4 KiB: AES-128-ECB with the master key; magic 0x13579753, checksum, the 16-byte key of the data
  blocks; the rest is padding (0xFF before encryption)
- data blocks, 4 KiB each up to the end of the flash: AES-128-ECB with the data key (constant parts + the key of the
  key block); 32 pages of 128 bytes. Page 0: magic 0x98761234 / 0x135726AB, checksum, block id, a bitmap of the index
  pages. An index page names a value, its length and the parts (block id, first page, last page) holding it; the
  other pages are data. A block id seen twice is a swap copy, the first one counts

*/

#if defined(LIBRETINY) || defined(TUYA_KV_HOST_TEST)

#include "tuya_kv_beken.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <memory>

namespace espurna {
namespace beken {
namespace tuya_kv {
namespace {

// ---------------------------------------------------------------------------------------------------------------------
// AES-128 decryption, the S-boxes are computed instead of typed in

struct Aes {
    uint8_t sbox[256];
    uint8_t inverse[256];
    uint8_t round_keys[176];
};

uint8_t xtime(uint8_t value) {
    return static_cast<uint8_t>((value << 1) ^ ((value & 0x80) ? 0x1b : 0));
}

uint8_t multiply(uint8_t a, uint8_t b) {
    uint8_t out { 0 };
    while (b) {
        if (b & 1) {
            out ^= a;
        }
        a = xtime(a);
        b >>= 1;
    }

    return out;
}

uint8_t rotate(uint8_t value, int bits) {
    return static_cast<uint8_t>((value << bits) | (value >> (8 - bits)));
}

void make_boxes(Aes& aes) {
    for (int value = 0; value < 256; ++value) {
        // multiplicative inverse in GF(2^8): value^254
        uint8_t inv { 0 };
        if (value) {
            uint8_t result { 1 };
            uint8_t base { static_cast<uint8_t>(value) };
            for (int exponent = 254; exponent; exponent >>= 1) {
                if (exponent & 1) {
                    result = multiply(result, base);
                }
                base = multiply(base, base);
            }
            inv = result;
        }

        const uint8_t s = static_cast<uint8_t>(
            inv ^ rotate(inv, 1) ^ rotate(inv, 2) ^ rotate(inv, 3) ^ rotate(inv, 4) ^ 0x63);
        aes.sbox[value] = s;
        aes.inverse[s] = static_cast<uint8_t>(value);
    }
}

void expand_key(Aes& aes, const uint8_t key[16]) {
    std::memcpy(aes.round_keys, key, 16);

    uint8_t rcon { 1 };
    for (int index = 16; index < 176; index += 4) {
        uint8_t word[4];
        std::memcpy(word, &aes.round_keys[index - 4], 4);
        if ((index % 16) == 0) {
            const uint8_t first = word[0];
            word[0] = static_cast<uint8_t>(aes.sbox[word[1]] ^ rcon);
            word[1] = aes.sbox[word[2]];
            word[2] = aes.sbox[word[3]];
            word[3] = aes.sbox[first];
            rcon = xtime(rcon);
        }

        for (int byte = 0; byte < 4; ++byte) {
            aes.round_keys[index + byte] = static_cast<uint8_t>(aes.round_keys[index - 16 + byte] ^ word[byte]);
        }
    }
}

void add_round_key(const Aes& aes, uint8_t state[16], int round) {
    for (int byte = 0; byte < 16; ++byte) {
        state[byte] ^= aes.round_keys[(round * 16) + byte];
    }
}

// state[row + 4 * column], in the order of the bytes
void inverse_shift_sub(const Aes& aes, uint8_t state[16]) {
    uint8_t copy[16];
    std::memcpy(copy, state, 16);
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            state[row + (4 * column)] = aes.inverse[copy[row + (4 * ((column - row + 4) % 4))]];
        }
    }
}

void inverse_mix(uint8_t state[16]) {
    for (int column = 0; column < 4; ++column) {
        uint8_t* a = &state[4 * column];
        const uint8_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
        a[0] = multiply(a0, 14) ^ multiply(a1, 11) ^ multiply(a2, 13) ^ multiply(a3, 9);
        a[1] = multiply(a0, 9) ^ multiply(a1, 14) ^ multiply(a2, 11) ^ multiply(a3, 13);
        a[2] = multiply(a0, 13) ^ multiply(a1, 9) ^ multiply(a2, 14) ^ multiply(a3, 11);
        a[3] = multiply(a0, 11) ^ multiply(a1, 13) ^ multiply(a2, 9) ^ multiply(a3, 14);
    }
}

void decrypt_block(const Aes& aes, const uint8_t in[16], uint8_t out[16]) {
    uint8_t state[16];
    std::memcpy(state, in, 16);

    add_round_key(aes, state, 10);
    for (int round = 9; round > 0; --round) {
        inverse_shift_sub(aes, state);
        add_round_key(aes, state, round);
        inverse_mix(state);
    }
    inverse_shift_sub(aes, state);
    add_round_key(aes, state, 0);

    std::memcpy(out, state, 16);
}

class Decryptor {
public:
    explicit Decryptor(const uint8_t key[16]) :
        _aes(new Aes)
    {
        make_boxes(*_aes);
        expand_key(*_aes, key);
    }

    ~Decryptor() {
        std::memset(_aes->round_keys, 0, sizeof(_aes->round_keys));
    }

    void decrypt(const uint8_t* in, uint8_t* out, size_t size) const {
        for (size_t offset = 0; offset + 16 <= size; offset += 16) {
            decrypt_block(*_aes, &in[offset], &out[offset]);
        }
    }

private:
    std::unique_ptr<Aes> _aes;
};

// ---------------------------------------------------------------------------------------------------------------------
// Store

constexpr uint8_t MasterKey[16] {
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 'a', 's', 'd', 'f', 'g', 'h'};
constexpr char KeyPart1[] = "8710_2M";
constexpr char KeyPart2[] = "HHRRQbyemofrtytf";

constexpr uint32_t MagicKey { 0x13579753 };
constexpr uint32_t MagicData1 { 0x98761234 };
constexpr uint32_t MagicData2 { 0x135726AB };

constexpr uint32_t BlockSize { 0x1000 };
constexpr uint32_t PageSize { 128 };
constexpr uint32_t Pages { BlockSize / PageSize };
constexpr uint32_t SearchSize { 0x20000 };

uint32_t le32(const uint8_t* data) {
    return static_cast<uint32_t>(data[0])
        | (static_cast<uint32_t>(data[1]) << 8)
        | (static_cast<uint32_t>(data[2]) << 16)
        | (static_cast<uint32_t>(data[3]) << 24);
}

uint16_t le16(const uint8_t* data) {
    return static_cast<uint16_t>(data[0] | (data[1] << 8));
}

struct Part {
    uint16_t block;
    uint8_t first;
    uint8_t last;
};

struct Index {
    uint32_t length { 0 };
    std::vector<Part> parts;
};

struct DataKey {
    uint8_t bytes[16];
};

DataKey make_data_key(const uint8_t key[16]) {
    DataKey out;
    for (int index = 0; index < 16; ++index) {
        out.bytes[index] = static_cast<uint8_t>(KeyPart1[index & 3] + KeyPart2[index] + key[index]);
    }
    return out;
}

class Store {
public:
    Store(FlashRead read, uint32_t offset, const DataKey& key) :
        _read(read),
        _offset(offset),
        _data(key.bytes)
    {}

    // block id -> flash offset, and the "chip-factory/" indexes
    void scan(uint32_t end) {
        uint8_t page[PageSize];
        for (uint32_t block = _offset + BlockSize; block + BlockSize <= end; block += BlockSize) {
            if (!read_page(block, 0, page)) {
                continue;
            }

            const auto magic = le32(&page[0]);
            if ((magic != MagicData1) && (magic != MagicData2)) {
                continue;
            }

            // the SDK moves blocks around at runtime (two reads of the same board had them at other offsets); a block id
            // seen twice is a copy in the middle of a swap: the data pages of the first one count, the names of both
            const auto id = le16(&page[8]);
            _blocks.emplace(id, block);

            const uint8_t map_size = page[14];
            if ((map_size == 0) || (15u + map_size > PageSize)) {
                continue;
            }

            uint8_t map[8] {};
            std::memcpy(map, &page[15], std::min<size_t>(map_size, sizeof(map)));

            for (uint32_t number = 1; number < Pages; ++number) {
                const uint32_t byte = number / 8;
                if ((byte >= map_size) || (byte >= sizeof(map)) || !(map[byte] & (1u << (number % 8)))) {
                    continue;
                }

                uint8_t index[PageSize];
                if (!read_page(block, number, index)) {
                    continue;
                }

                if ((le16(&index[8]) != id) || (index[10] != number)) {
                    continue;
                }

                const uint8_t name_length = index[17];
                const uint16_t parts = le16(&index[11]);
                if (!name_length || (18u + name_length + (4u * parts) > PageSize)) {
                    continue;
                }

                std::string name(reinterpret_cast<const char*>(&index[18]),
                    strnlen(reinterpret_cast<const char*>(&index[18]), name_length));
                if (name.compare(0, 13, "chip-factory/") != 0) {
                    continue;
                }

                if (_indexes.count(name)) {
                    continue;
                }

                Index entry;
                entry.length = le32(&index[4]);
                for (uint16_t part = 0; part < parts; ++part) {
                    const uint8_t* data = &index[18 + name_length + (4 * part)];
                    entry.parts.push_back(Part{le16(data), data[2], data[3]});
                }

                _indexes.emplace(std::move(name), std::move(entry));
            }
        }
    }

    // value without the zero padding
    bool value(const char* name, std::vector<uint8_t>& out) const {
        const auto it = _indexes.find(name);
        if (it == _indexes.end()) {
            return false;
        }

        out.clear();
        uint8_t page[PageSize];
        for (const auto& part : it->second.parts) {
            const auto block = _blocks.find(part.block);
            if (block == _blocks.end()) {
                return false;
            }

            for (uint32_t number = part.first; number <= part.last; ++number) {
                if ((number == 0) || (number >= Pages) || !read_page(block->second, number, page)) {
                    return false;
                }
                out.insert(out.end(), page, page + PageSize);
            }
        }

        if (out.size() < it->second.length) {
            return false;
        }

        out.resize(it->second.length);
        while (!out.empty() && (out.back() == 0)) {
            out.pop_back();
        }

        return true;
    }

private:
    bool read_page(uint32_t block, uint32_t number, uint8_t out[PageSize]) const {
        uint8_t raw[PageSize];
        if (!_read(block + (number * PageSize), raw, sizeof(raw))) {
            return false;
        }
        _data.decrypt(raw, out, PageSize);
        return true;
    }

    FlashRead _read;
    uint32_t _offset;
    Decryptor _data;
    std::map<uint16_t, uint32_t> _blocks;
    std::map<std::string, Index> _indexes;
};

// The key block: decrypted with the master key it starts with its magic, and its padding is 0xFF
bool find_store(FlashRead read, uint32_t flash_size, const Decryptor& master, uint32_t& offset, uint8_t key[16]) {
    const uint32_t start = (flash_size > SearchSize) ? (flash_size - SearchSize) : 0;
    for (uint32_t block = start; block + BlockSize <= flash_size; block += BlockSize) {
        uint8_t raw[48];
        uint8_t plain[48];
        if (!read(block, raw, sizeof(raw))) {
            continue;
        }
        master.decrypt(raw, plain, sizeof(plain));

        if (le32(&plain[0]) != MagicKey) {
            continue;
        }

        bool padding { true };
        for (size_t index = 32; index < 48; ++index) {
            padding = padding && (plain[index] == 0xff);
        }

        if (!padding) {
            continue;
        }

        offset = block;
        std::memcpy(key, &plain[8], 16);
        std::memset(plain, 0, sizeof(plain));
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------------------------------------------------
// Values

std::string text(const std::vector<uint8_t>& value) {
    return std::string(value.begin(), value.end());
}

size_t json_value(const std::string& json, const char* key) {
    const std::string pattern = std::string("\"") + key + "\":";
    auto position = json.find(pattern);
    if (position == std::string::npos) {
        return std::string::npos;
    }

    position += pattern.size();
    while ((position < json.size()) && (json[position] == ' ')) {
        ++position;
    }

    return position;
}

// "key": "string", the first one in the text (the matter-resource keys are unique)
bool json_string(const std::string& json, const char* key, std::string& out) {
    const auto position = json_value(json, key);
    if ((position == std::string::npos) || (position >= json.size()) || (json[position] != '"')) {
        return false;
    }

    const auto end = json.find('"', position + 1);
    if (end == std::string::npos) {
        return false;
    }

    out = json.substr(position + 1, end - position - 1);
    return true;
}

// "key": number
bool json_number(const std::string& json, const char* key, uint32_t& out) {
    auto position = json_value(json, key);
    if (position == std::string::npos) {
        return false;
    }

    uint32_t value { 0 };
    bool digits { false };
    while ((position < json.size()) && (json[position] >= '0') && (json[position] <= '9')) {
        value = (value * 10) + static_cast<uint32_t>(json[position] - '0');
        digits = true;
        ++position;
    }

    if (digits) {
        out = value;
    }

    return digits;
}

bool pem_to_der(const std::vector<uint8_t>& pem, std::vector<uint8_t>& out) {
    const auto value = text(pem);
    const auto begin = value.find("-----BEGIN ");
    if (begin == std::string::npos) {
        return false;
    }

    const auto body = value.find("-----", begin + 11);
    const auto end = value.find("-----END ");
    if ((body == std::string::npos) || (end == std::string::npos) || (end < body + 5)) {
        return false;
    }

    return base64_decode(value.data() + body + 5, end - body - 5, out);
}

// SEC1 ECPrivateKey: SEQUENCE { INTEGER 1, OCTET STRING (32) ... }
bool sec1_private_key(const std::vector<uint8_t>& der, std::vector<uint8_t>& out) {
    if ((der.size() < 2) || (der[0] != 0x30)) {
        return false;
    }

    const size_t position = (der[1] & 0x80) ? (2u + (der[1] & 0x7f)) : 2u;
    if ((position + 5 + 32 > der.size())
        || (der[position] != 0x02) || (der[position + 1] != 0x01) || (der[position + 2] != 0x01)
        || (der[position + 3] != 0x04) || (der[position + 4] != 0x20)) {
        return false;
    }

    out.assign(der.begin() + static_cast<long>(position + 5), der.begin() + static_cast<long>(position + 5 + 32));
    return true;
}

// SubjectPublicKeyInfo of P-256: ... BIT STRING (66) { 0, 0x04 X Y }
bool spki_public_key(const std::vector<uint8_t>& der, std::vector<uint8_t>& out) {
    if (der.size() < 68) {
        return false;
    }

    const auto key = der.size() - 65;
    if ((der[key] != 0x04) || (der[key - 3] != 0x03) || (der[key - 2] != 0x42) || (der[key - 1] != 0x00)) {
        return false;
    }

    out.assign(der.begin() + static_cast<long>(key), der.end());
    return true;
}

void wipe(std::vector<uint8_t>& value) {
    std::fill(value.begin(), value.end(), 0);
}

void wipe(std::string& value) {
    std::fill(value.begin(), value.end(), '\0');
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------------

bool aes128_ecb_decrypt(const uint8_t key[16], const uint8_t* in, uint8_t* out, size_t size) {
    if (size % 16) {
        return false;
    }

    Decryptor(key).decrypt(in, out, size);
    return true;
}

bool base64_decode(const char* in, size_t size, std::vector<uint8_t>& out) {
    out.clear();
    uint32_t buffer { 0 };
    int bits { 0 };
    for (size_t index = 0; index < size; ++index) {
        const char c = in[index];
        int value;
        if ((c >= 'A') && (c <= 'Z')) {
            value = c - 'A';
        } else if ((c >= 'a') && (c <= 'z')) {
            value = c - 'a' + 26;
        } else if ((c >= '0') && (c <= '9')) {
            value = c - '0' + 52;
        } else if (c == '+') {
            value = 62;
        } else if (c == '/') {
            value = 63;
        } else if ((c == '=') || (c == '\r') || (c == '\n') || (c == ' ') || (c == '\t')) {
            continue;
        } else {
            return false;
        }

        buffer = ((buffer << 6) | static_cast<uint32_t>(value)) & 0xffffff;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<uint8_t>((buffer >> bits) & 0xff));
        }
    }

    return !out.empty();
}

std::string base64_encode(const uint8_t* data, size_t size) {
    static const char Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((size + 2) / 3) * 4);
    for (size_t index = 0; index < size; index += 3) {
        const uint32_t chunk = (static_cast<uint32_t>(data[index]) << 16)
            | ((index + 1 < size) ? (static_cast<uint32_t>(data[index + 1]) << 8) : 0)
            | ((index + 2 < size) ? static_cast<uint32_t>(data[index + 2]) : 0);
        out += Alphabet[(chunk >> 18) & 0x3f];
        out += Alphabet[(chunk >> 12) & 0x3f];
        out += (index + 1 < size) ? Alphabet[(chunk >> 6) & 0x3f] : '=';
        out += (index + 2 < size) ? Alphabet[chunk & 0x3f] : '=';
    }

    return out;
}

bool load_matter_factory(FlashRead read, uint32_t flash_size, MatterFactory& out) {
    uint32_t offset { 0 };
    uint8_t key[16];
    {
        const Decryptor master(MasterKey);
        if (!find_store(read, flash_size, master, offset, key)) {
            return false;
        }
    }

    Store store(read, offset, make_data_key(key));
    std::memset(key, 0, sizeof(key));
    store.scan(flash_size);

    std::vector<uint8_t> value;
    if (!store.value("chip-factory/dac-crt", value) || !pem_to_der(value, out.dac)) {
        return false;
    }

    if (!store.value("chip-factory/pai-crt", value) || !pem_to_der(value, out.pai)) {
        return false;
    }

    std::vector<uint8_t> der;
    const bool secret = store.value("chip-factory/private-key", value) && pem_to_der(value, der)
        && sec1_private_key(der, out.dac_private);
    wipe(der);
    wipe(value);
    if (!secret) {
        return false;
    }

    if (!store.value("chip-factory/public-key", value) || !pem_to_der(value, der)
        || !spki_public_key(der, out.dac_public)) {
        return false;
    }

    if (!store.value("chip-factory/matter-resource", value)) {
        return false;
    }

    auto json = text(value);
    wipe(value);

    std::string declaration;
    std::string w0;
    std::string l;
    uint32_t vendor { 0 };
    uint32_t product { 0 };
    uint32_t discriminator { 0 };
    uint32_t hardware { 0 };
    bool complete = json_string(json, "cd", declaration)
        && base64_decode(declaration.data(), declaration.size(), out.declaration)
        && json_string(json, "salt", out.salt)
        && json_string(json, "w0", w0) && json_string(json, "l", l)
        && json_number(json, "iterations", out.iterations)
        && json_number(json, "passcode", out.passcode)
        && json_number(json, "discriminator", discriminator)
        && json_number(json, "vendorId", vendor)
        && json_number(json, "productId", product);

    std::vector<uint8_t> verifier;
    std::vector<uint8_t> point;
    complete = complete
        && base64_decode(w0.data(), w0.size(), verifier) && base64_decode(l.data(), l.size(), point)
        && (verifier.size() == 32) && (point.size() == 65);
    if (complete) {
        verifier.insert(verifier.end(), point.begin(), point.end());
        out.verifier = base64_encode(verifier.data(), verifier.size());
    }
    wipe(verifier);
    wipe(point);
    wipe(w0);
    wipe(l);

    if (complete) {
        json_string(json, "vendorName", out.vendor_name);
        json_string(json, "productName", out.product_name);
        json_string(json, "serialNumber", out.serial);
        json_number(json, "hardwareVersion", hardware);

        out.discriminator = static_cast<uint16_t>(discriminator);
        out.vendor_id = static_cast<uint16_t>(vendor);
        out.product_id = static_cast<uint16_t>(product);
        out.hardware_version = static_cast<uint16_t>(hardware);
        out.offset = offset;
    }
    wipe(json);

    return complete;
}

} // namespace tuya_kv
} // namespace beken
} // namespace espurna

#endif
