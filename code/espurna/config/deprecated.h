#pragma once

// 1.13.3 added TELNET_PASSWORD build-only flag
// 1.13.4 replaces it with TELNET_AUTHENTICATION runtime setting default
// TODO warning should be removed eventually
#ifdef TELNET_PASSWORD
#warning TELNET_PASSWORD is deprecated! Please replace it with TELNET_AUTHENTICATION
#define TELNET_AUTHENTICATION TELNET_PASSWORD
#endif
