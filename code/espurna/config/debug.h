#define DEBUG_MESSAGE_MAX_LENGTH    80

#ifdef SONOFF_DUAL
#undef DEBUG_PORT
#endif

#if defined(DEBUG_PORT) | defined(DEBUG_UDP_IP)
    #define DEBUG_MSG(...) debugSend(__VA_ARGS__)
    #define DEBUG_MSG_P(...) debugSend_P(__VA_ARGS__)
#endif

#ifndef DEBUG_MSG
    #define DEBUG_MSG(...)
    #define DEBUG_MSG_P(...)
#endif
