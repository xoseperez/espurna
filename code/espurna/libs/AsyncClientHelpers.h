// -----------------------------------------------------------------------------
// AsyncClient helpers
// -----------------------------------------------------------------------------

#pragma once

enum class AsyncClientState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting
};
