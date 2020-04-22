// -----------------------------------------------------------------------------
// AsyncClient helpers
// -----------------------------------------------------------------------------

#pragma once

#include <ESPAsyncTCP.h>

enum class AsyncClientState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting
};
    

