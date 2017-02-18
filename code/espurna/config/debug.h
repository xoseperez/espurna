#ifdef DEBUG_PORT
    #define DEBUG_MSG(...) DEBUG_PORT.printf( __VA_ARGS__ )
    #define DEBUG_MSG_P(...) { char buffer[81]; snprintf_P(buffer, 80, __VA_ARGS__ ); DEBUG_PORT.printf( buffer ); }
#else
    #define DEBUG_MSG(...)
    #define DEBUG_MSG_P(...)
#endif
