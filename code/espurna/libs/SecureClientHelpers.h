// -----------------------------------------------------------------------------
// WiFiClientSecure validation helpers
// -----------------------------------------------------------------------------

#pragma once

#include "../espurna.h"

#if SECURE_CLIENT != SECURE_CLIENT_NONE

#include "../ntp.h"

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
#include "ntp_timelib.h"
#include <WiFiClientSecureBearSSL.h>
#elif SECURE_CLIENT == SECURE_CLIENT_AXTLS
#include <WiFiClientSecureAxTLS.h>
#endif

namespace SecureClientHelpers {

using host_callback_f = std::function<String()>;
using check_callback_f = std::function<int()>;
using fp_callback_f = std::function<String()>;
using cert_callback_f = std::function<const char*()>;
using mfln_callback_f = std::function<uint16_t()>;

// TODO: workaround for `multiple definition of `SecureClientHelpers::_secureClientCheckAsString(int);'
inline const char * _secureClientCheckAsString(int check) {
    switch (check) {
        case SECURE_CLIENT_CHECK_NONE: return "no validation";
        case SECURE_CLIENT_CHECK_FINGERPRINT: return "fingerprint validation";
        case SECURE_CLIENT_CHECK_CA: return "CA validation";
        default: return "unknown";
    }
}

#if SECURE_CLIENT == SECURE_CLIENT_AXTLS
using SecureClientClass = axTLS::WiFiClientSecure;

struct SecureClientConfig {
    const char* tag;
    host_callback_f on_host;
    check_callback_f on_check;
    fp_callback_f on_fingerprint;
    bool debug;
};

struct SecureClientChecks {

    SecureClientChecks(SecureClientConfig& config) :
        config(config)
    {}

    int getCheck() {
        return (config.on_check) ? config.on_check() : (SECURE_CLIENT_CHECK);
    }

    bool beforeConnected(SecureClientClass& client) {
        return true;
    }

    // Special condition for legacy client!
    // Otherwise, we are required to connect twice. And it is deemed broken & deprecated anyways...
    bool afterConnected(SecureClientClass& client) {
        bool result = false;

        int check = getCheck();

        if(config.debug) {
            DEBUG_MSG_P(PSTR("[%s] Using SSL check type: %s\n"), config.tag, _secureClientCheckAsString(check));
        }

        if (check == SECURE_CLIENT_CHECK_NONE) {
            if (config.debug) DEBUG_MSG_P(PSTR("[%s] !!! Secure connection will not be validated !!!\n"), config.tag);
            result = true;
        } else if (check == SECURE_CLIENT_CHECK_FINGERPRINT) {
            if (config.on_fingerprint) {
                char _buffer[60] = {0};
                if (config.on_fingerprint && config.on_host && sslFingerPrintChar(config.on_fingerprint().c_str(), _buffer)) {
                    result = client.verify(_buffer, config.on_host().c_str());
                }
                if (!result) DEBUG_MSG_P(PSTR("[%s] Wrong fingerprint, cannot connect\n"), config.tag);
            }
        } else if (check == SECURE_CLIENT_CHECK_CA) {
           if (config.debug) DEBUG_MSG_P(PSTR("[%s] CA verification is not supported with axTLS client\n"), config.tag);
        }

        return result;
    }

    SecureClientConfig& config;
    bool debug;

};
#endif // SECURE_CLIENT_AXTLS

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL

using SecureClientClass = BearSSL::WiFiClientSecure;

struct SecureClientConfig {
    const char* tag;
    check_callback_f on_check;
    cert_callback_f on_certificate;
    fp_callback_f on_fingerprint;
    mfln_callback_f on_mfln;
    bool debug;
};

struct SecureClientChecks {

    SecureClientChecks(SecureClientConfig& config) :
        config(config)
    {}

    int getCheck() {
        return (config.on_check) ? config.on_check() : (SECURE_CLIENT_CHECK);
    }

    bool prepareMFLN(SecureClientClass& client) {
        const uint16_t requested_mfln = (config.on_mfln) ? config.on_mfln() : (SECURE_CLIENT_MFLN);
        bool result = false;
        switch (requested_mfln) {
            // default, do nothing
            case 0:
                result = true;
                break;
            // match valid sizes only
            case 512:
            case 1024:
            case 2048:
            case 4096:
            {
                client.setBufferSizes(requested_mfln, requested_mfln);
                result = true;
                if (config.debug) {
                    DEBUG_MSG_P(PSTR("[%s] MFLN buffer size set to %u\n"), config.tag, requested_mfln);
                }
                break;
            }
            default:
            {
                if (config.debug) {
                    DEBUG_MSG_P(PSTR("[%s] Warning: MFLN buffer size must be one of 512, 1024, 2048 or 4096\n"), config.tag);
                }
            }
        }

        return result;
    }

    bool beforeConnected(SecureClientClass& client) {
        int check = getCheck();
        bool settime = (check == SECURE_CLIENT_CHECK_CA);

        if(config.debug) {
            DEBUG_MSG_P(PSTR("[%s] Using SSL check type: %s\n"), config.tag, _secureClientCheckAsString(check));
        }

        if (!ntpSynced() && settime) {
            if (config.debug) DEBUG_MSG_P(PSTR("[%s] Time not synced! Cannot use CA validation\n"), config.tag);
            return false;
        }

        prepareMFLN(client);

        if (check == SECURE_CLIENT_CHECK_NONE) {
            if (config.debug) DEBUG_MSG_P(PSTR("[%s] !!! Secure connection will not be validated !!!\n"), config.tag);
            client.setInsecure();
        } else if (check == SECURE_CLIENT_CHECK_FINGERPRINT) {
            uint8_t _buffer[20] = {0};
            if (config.on_fingerprint && sslFingerPrintArray(config.on_fingerprint().c_str(), _buffer)) {
                client.setFingerprint(_buffer);
            }
        } else if (check == SECURE_CLIENT_CHECK_CA) {
            client.setX509Time(now());
            if (!certs.getCount()) {
                if (config.on_certificate) certs.append(config.on_certificate());
            }
            client.setTrustAnchors(&certs);
        }

        return true;
    }

    bool afterConnected(SecureClientClass&) {
        return true;
    }

    bool debug;

    SecureClientConfig& config;

    BearSSL::X509List certs;

};
#endif // SECURE_CLIENT_BEARSSL

class SecureClient {

    public:

    SecureClient(SecureClientConfig& config) :
        _config(config),
        _checks(_config),
        _client(std::make_unique<SecureClientClass>())
    {}

    bool afterConnected() {
        return _checks.afterConnected(get());
    }

    bool beforeConnected() {
        return _checks.beforeConnected(get());
    }

    SecureClientClass& get() {
        return *_client.get();
    }

    private:

    SecureClientConfig _config;
    SecureClientChecks _checks;
    std::unique_ptr<SecureClientClass> _client;

};

};

using SecureClientConfig = SecureClientHelpers::SecureClientConfig;
using SecureClientChecks = SecureClientHelpers::SecureClientChecks;
using SecureClient = SecureClientHelpers::SecureClient;

#endif // SECURE_CLIENT != SECURE_CLIENT_NONE
