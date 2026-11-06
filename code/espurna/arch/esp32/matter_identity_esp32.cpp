/*

MATTER BACKEND FOR ESP32: OWN BOARD IDENTITY

Each board makes its own commissioning data and device attestation once, instead of the SDK test values every image
shares (setup passcode 20202021, discriminator 0xF00, development DAC 0xFFF1/0x8000):
- setup passcode and discriminator, random;
- a P-256 device attestation key and its certificate (DAC). The certificate is the SDK development DAC (VID 0xFFF1,
  PID 0x8000) with this board's serial number, public key and key identifier, signed by the PAI the SDK example
  provider serves, "Matter Dev PAI 0xFFF1 no PID". That PAI and its key are public (connectedhomeip,
  credentials/development/attestation): controllers that accept the SDK example devices accept this one, it is not a
  certified identity. The PAI certificate and the certification declaration stay the SDK ones.

Kept in the NVS namespace "espurna-matter" (default 'nvs' partition): an OTA update, the safeboot, a serial flash of an
image and a Matter factory reset leave it, so the QR code / pairing code stays the same. Erasing the whole flash
makes a new one (the codes change, the controllers have to add the board again)

*/

#include "espurna.h"

#if defined(ARDUINO_ARCH_ESP32) && MATTER_SUPPORT

#include "matter_identity_esp32.h"

#include <credentials/DeviceAttestationCredsProvider.h>
#include <crypto/CHIPCryptoPAL.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/Span.h>
#include <setup_payload/SetupPayload.h>

#include <esp_random.h>
#include <nvs.h>

#include <cstring>

namespace espurna {
namespace matter {
namespace backend {
namespace identity {
namespace {

// SDK development DAC FFF1/8000, the part to be signed. What differs per board is patched in at the offsets below
constexpr uint8_t DacTbs[] {
    0x30, 0x82, 0x01, 0x8e, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x08, 0x23, 0x8a, 0x64, 0x7b, 0xbc,
    0x4c, 0x30, 0xdd, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x30,
    0x3d, 0x31, 0x25, 0x30, 0x23, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x1c, 0x4d, 0x61, 0x74, 0x74,
    0x65, 0x72, 0x20, 0x44, 0x65, 0x76, 0x20, 0x50, 0x41, 0x49, 0x20, 0x30, 0x78, 0x46, 0x46, 0x46,
    0x31, 0x20, 0x6e, 0x6f, 0x20, 0x50, 0x49, 0x44, 0x31, 0x14, 0x30, 0x12, 0x06, 0x0a, 0x2b, 0x06,
    0x01, 0x04, 0x01, 0x82, 0xa2, 0x7c, 0x02, 0x01, 0x0c, 0x04, 0x46, 0x46, 0x46, 0x31, 0x30, 0x20,
    0x17, 0x0d, 0x32, 0x32, 0x30, 0x32, 0x30, 0x35, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18,
    0x0f, 0x39, 0x39, 0x39, 0x39, 0x31, 0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35, 0x39, 0x5a,
    0x30, 0x53, 0x31, 0x25, 0x30, 0x23, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x1c, 0x4d, 0x61, 0x74,
    0x74, 0x65, 0x72, 0x20, 0x44, 0x65, 0x76, 0x20, 0x44, 0x41, 0x43, 0x20, 0x30, 0x78, 0x46, 0x46,
    0x46, 0x31, 0x2f, 0x30, 0x78, 0x38, 0x30, 0x30, 0x30, 0x31, 0x14, 0x30, 0x12, 0x06, 0x0a, 0x2b,
    0x06, 0x01, 0x04, 0x01, 0x82, 0xa2, 0x7c, 0x02, 0x01, 0x0c, 0x04, 0x46, 0x46, 0x46, 0x31, 0x31,
    0x14, 0x30, 0x12, 0x06, 0x0a, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xa2, 0x7c, 0x02, 0x02, 0x0c,
    0x04, 0x38, 0x30, 0x30, 0x30, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d,
    0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04,
    0x62, 0xdb, 0x16, 0xba, 0xde, 0xa3, 0x26, 0xa6, 0xdb, 0x84, 0x81, 0x4a, 0x06, 0x3f, 0xc6, 0xc7,
    0xe9, 0xe2, 0xb1, 0x01, 0xb7, 0x21, 0x64, 0x8e, 0xba, 0x4e, 0x5a, 0xc8, 0x40, 0xf5, 0xda, 0x30,
    0x1e, 0xe6, 0x18, 0x12, 0x4e, 0xb4, 0x18, 0x0e, 0x2f, 0xc3, 0xa2, 0x04, 0x7a, 0x56, 0x4b, 0xa9,
    0xbc, 0xfa, 0x0b, 0xf7, 0x1f, 0x60, 0xce, 0x89, 0x30, 0xf1, 0xe7, 0xf6, 0x6e, 0xc8, 0xd7, 0x28,
    0xa3, 0x60, 0x30, 0x5e, 0x30, 0x0c, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x02,
    0x30, 0x00, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01, 0xff, 0x04, 0x04, 0x03, 0x02,
    0x07, 0x80, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0xbc, 0xf7, 0xb0,
    0x07, 0x49, 0x70, 0x63, 0x60, 0x6a, 0x26, 0xbe, 0x4e, 0x08, 0x7c, 0x59, 0x56, 0x87, 0x74, 0x5a,
    0x5a, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x63, 0x54,
    0x0e, 0x47, 0xf6, 0x4b, 0x1c, 0x38, 0xd1, 0x38, 0x84, 0xa4, 0x62, 0xd1, 0x6c, 0x19, 0x5d, 0x8f,
    0xfb, 0x3c,
};

constexpr size_t SerialOffset { 11 };
constexpr size_t SerialLength { 8 };
constexpr size_t PublicKeyOffset { 239 };
constexpr size_t KeyIdOffset { 349 };

// ecdsa-with-SHA256
constexpr uint8_t SignatureAlgorithm[] {
    0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02,
};

// Matter-Development-PAI-FFF1-noPID key pair (public development credentials of connectedhomeip)
constexpr uint8_t PaiPublicKey[] {
    0x04, 0x41, 0x9a, 0x93, 0x15, 0xc2, 0x17, 0x3e, 0x0c, 0x8c, 0x87, 0x6d, 0x03, 0xcc, 0xfc, 0x94,
    0x48, 0x52, 0x64, 0x7f, 0x7f, 0xec, 0x5e, 0x50, 0x82, 0xf4, 0x05, 0x99, 0x28, 0xec, 0xa8, 0x94,
    0xc5, 0x94, 0x15, 0x13, 0x09, 0xac, 0x63, 0x1e, 0x4c, 0xb0, 0x33, 0x92, 0xaf, 0x68, 0x4b, 0x0b,
    0xaf, 0xb7, 0xe6, 0x5b, 0x3b, 0x81, 0x62, 0xc2, 0xf5, 0x2b, 0xf9, 0x31, 0xb8, 0xe7, 0x7a, 0xaa,
    0x82,
};

constexpr uint8_t PaiPrivateKey[] {
    0xc1, 0xab, 0x5f, 0xe2, 0x84, 0xb4, 0xc1, 0x89, 0x40, 0xa5, 0x31, 0x61, 0xf8, 0x06, 0x94, 0x40,
    0x50, 0xa1, 0x69, 0x7b, 0x71, 0x76, 0x1d, 0x38, 0x53, 0x37, 0xa8, 0xa3, 0xcd, 0x09, 0x5c, 0x34,
};

constexpr char Namespace[] = "espurna-matter";
constexpr char KeyIdentity[] = "identity";
constexpr char KeyCertificate[] = "dac";

constexpr uint32_t Magic { 0x45494d31 }; // "EMI1"
// DAC made under the PAI of this version; another one (or none) is made again, the codes stay
constexpr uint8_t KeyVersion { 2 };

struct Stored {
    uint32_t magic;
    uint32_t passcode;
    uint16_t discriminator;
    uint8_t key_version;
    uint8_t serial[SerialLength];
    uint8_t keypair[chip::Crypto::kP256_PublicKey_Length + chip::Crypto::kP256_PrivateKey_Length];
};

Stored stored {};
bool loaded { false };
bool installed { false };

bool read_identity() {
    nvs_handle_t handle;
    if (nvs_open(Namespace, NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    size_t size { sizeof(stored) };
    const auto err = nvs_get_blob(handle, KeyIdentity, &stored, &size);
    nvs_close(handle);

    return (err == ESP_OK) && (size == sizeof(stored)) && (stored.magic == Magic);
}

bool write_blobs(const uint8_t* certificate, size_t certificate_size) {
    nvs_handle_t handle;
    if (nvs_open(Namespace, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    }

    auto err = ESP_OK;
    if (certificate) {
        err = nvs_set_blob(handle, KeyCertificate, certificate, certificate_size);
    }
    if (err == ESP_OK) {
        err = nvs_set_blob(handle, KeyIdentity, &stored, sizeof(stored));
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);

    return err == ESP_OK;
}

void make_codes() {
    stored = Stored{};
    stored.magic = Magic;

    do {
        stored.passcode = (esp_random() % 99999998u) + 1u;
    } while (!chip::SetupPayload::IsValidSetupPIN(stored.passcode));

    stored.discriminator = esp_random() & 0xfff;

    esp_fill_random(stored.serial, sizeof(stored.serial));
    // positive, and minimal as a DER integer
    stored.serial[0] = (stored.serial[0] & 0x3f) | 0x40;
}

// New key, its certificate signed with the development PAI key. The certificate is only read while commissioning: NVS keeps
// it, not the heap
bool make_certificate() {
    chip::Crypto::P256Keypair key;
    if (key.Initialize(chip::Crypto::ECPKeyTarget::ECDSA) != CHIP_NO_ERROR) {
        return false;
    }

    chip::Crypto::P256SerializedKeypair serialized;
    if ((key.Serialize(serialized) != CHIP_NO_ERROR) || (serialized.Length() != sizeof(stored.keypair))) {
        return false;
    }
    std::memcpy(stored.keypair, serialized.ConstBytes(), sizeof(stored.keypair));

    uint8_t tbs[sizeof(DacTbs)];
    std::memcpy(tbs, DacTbs, sizeof(tbs));
    std::memcpy(&tbs[SerialOffset], stored.serial, SerialLength);

    const auto& pub = key.Pubkey();
    std::memcpy(&tbs[PublicKeyOffset], pub.ConstBytes(), chip::Crypto::kP256_PublicKey_Length);

    uint8_t key_id[chip::Crypto::kSHA1_Hash_Length];
    if (chip::Crypto::Hash_SHA1(pub.ConstBytes(), chip::Crypto::kP256_PublicKey_Length, key_id) != CHIP_NO_ERROR) {
        return false;
    }
    std::memcpy(&tbs[KeyIdOffset], key_id, sizeof(key_id));

    chip::Crypto::P256Keypair pai;
    chip::Crypto::P256SerializedKeypair pai_serialized;
    std::memcpy(pai_serialized.Bytes(), PaiPublicKey, sizeof(PaiPublicKey));
    std::memcpy(pai_serialized.Bytes() + sizeof(PaiPublicKey), PaiPrivateKey, sizeof(PaiPrivateKey));
    pai_serialized.SetLength(sizeof(PaiPublicKey) + sizeof(PaiPrivateKey));
    if (pai.Deserialize(pai_serialized) != CHIP_NO_ERROR) {
        return false;
    }

    chip::Crypto::P256ECDSASignature raw;
    if (pai.ECDSA_sign_msg(tbs, sizeof(tbs), raw) != CHIP_NO_ERROR) {
        return false;
    }

    uint8_t signature[chip::Crypto::kMax_ECDSA_Signature_Length_Der];
    chip::MutableByteSpan signature_span(signature);
    if (chip::Crypto::EcdsaRawSignatureToAsn1(chip::Crypto::kP256_FE_Length,
            chip::ByteSpan(raw.ConstBytes(), raw.Length()), signature_span) != CHIP_NO_ERROR) {
        return false;
    }

    // SEQUENCE { tbs, signature algorithm, BIT STRING { 0 unused bits, signature } }
    const size_t bits = 1 + signature_span.size();
    const size_t body = sizeof(tbs) + sizeof(SignatureAlgorithm) + 2 + bits;

    uint8_t certificate[4 + sizeof(DacTbs) + sizeof(SignatureAlgorithm) + 3 + sizeof(signature)];
    size_t length { 0 };
    certificate[length++] = 0x30;
    certificate[length++] = 0x82;
    certificate[length++] = (body >> 8) & 0xff;
    certificate[length++] = body & 0xff;
    std::memcpy(&certificate[length], tbs, sizeof(tbs));
    length += sizeof(tbs);
    std::memcpy(&certificate[length], SignatureAlgorithm, sizeof(SignatureAlgorithm));
    length += sizeof(SignatureAlgorithm);
    certificate[length++] = 0x03;
    certificate[length++] = bits;
    certificate[length++] = 0x00;
    std::memcpy(&certificate[length], signature_span.data(), signature_span.size());
    length += signature_span.size();

    stored.key_version = KeyVersion;
    if (!write_blobs(certificate, length)) {
        stored.key_version = 0;
        return false;
    }

    return true;
}

// Own DAC and key, the rest from the provider the stack had (SDK example: development PAI, test certification
// declaration)
class Provider final : public chip::Credentials::DeviceAttestationCredentialsProvider {
public:
    explicit Provider(chip::Credentials::DeviceAttestationCredentialsProvider* base) :
        _base(base)
    {}

    CHIP_ERROR GetCertificationDeclaration(chip::MutableByteSpan& out) override {
        return _base->GetCertificationDeclaration(out);
    }

    CHIP_ERROR GetFirmwareInformation(chip::MutableByteSpan& out) override {
        return _base->GetFirmwareInformation(out);
    }

    CHIP_ERROR GetProductAttestationIntermediateCert(chip::MutableByteSpan& out) override {
        return _base->GetProductAttestationIntermediateCert(out);
    }

    CHIP_ERROR GetDeviceAttestationCert(chip::MutableByteSpan& out) override {
        nvs_handle_t handle;
        if (nvs_open(Namespace, NVS_READONLY, &handle) != ESP_OK) {
            return CHIP_ERROR_PERSISTED_STORAGE_FAILED;
        }

        size_t size = out.size();
        const auto err = nvs_get_blob(handle, KeyCertificate, out.data(), &size);
        nvs_close(handle);

        if (err != ESP_OK) {
            return CHIP_ERROR_PERSISTED_STORAGE_FAILED;
        }

        out.reduce_size(size);
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SignWithDeviceAttestationKey(const chip::ByteSpan& message, chip::MutableByteSpan& out) override {
        chip::Crypto::P256Keypair key;
        chip::Crypto::P256SerializedKeypair serialized;
        std::memcpy(serialized.Bytes(), stored.keypair, sizeof(stored.keypair));
        serialized.SetLength(sizeof(stored.keypair));
        ReturnErrorOnFailure(key.Deserialize(serialized));

        chip::Crypto::P256ECDSASignature signature;
        ReturnErrorOnFailure(key.ECDSA_sign_msg(message.data(), message.size(), signature));

        return chip::CopySpanToMutableSpan(chip::ByteSpan(signature.ConstBytes(), signature.Length()), out);
    }

private:
    chip::Credentials::DeviceAttestationCredentialsProvider* _base;
};

} // namespace

bool load(uint32_t& passcode, uint16_t& discriminator) {
    if (!loaded) {
        if (!read_identity()) {
            make_codes();
            if (!write_blobs(nullptr, 0)) {
                return false;
            }
        }
        loaded = true;
    }

    passcode = stored.passcode;
    discriminator = stored.discriminator;

    return true;
}

bool install() {
    if (installed) {
        return true;
    }

    if (!loaded) {
        return false;
    }

    if ((stored.key_version != KeyVersion) && !make_certificate()) {
        return false;
    }

    auto* base = chip::Credentials::GetDeviceAttestationCredentialsProvider();
    if (!base) {
        return false;
    }

    chip::Credentials::SetDeviceAttestationCredentialsProvider(new Provider(base));
    installed = true;

    return true;
}

String status() {
    if (!loaded) {
        return F("SDK test codes and DAC");
    }

    if (!installed) {
        return F("own codes, SDK test DAC");
    }

    String out(F("own codes and DAC, serial "));
    static constexpr char Hex[] = "0123456789abcdef";
    for (const auto byte : stored.serial) {
        out += Hex[byte >> 4];
        out += Hex[byte & 0xf];
    }

    return out;
}

} // namespace identity
} // namespace backend
} // namespace matter
} // namespace espurna

#endif
