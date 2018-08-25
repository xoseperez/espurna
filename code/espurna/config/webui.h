// -----------------------------------------------------------------------------
// WEB UI IMAGE
// -----------------------------------------------------------------------------

#define WEBUI_IMAGE_SMALL      0
#define WEBUI_IMAGE_LIGHT      1
#define WEBUI_IMAGE_SENSOR     2
#define WEBUI_IMAGE_RFBRIDGE   4
#define WEBUI_IMAGE_RFM69      8
#define WEBUI_IMAGE_FULL       15

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    #ifdef WEBUI_IMAGE
        #undef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_FULL
    #else
        #define WEBUI_IMAGE    WEBUI_IMAGE_LIGHT
    #endif
#endif

#if SENSOR_SUPPORT == 1
    #ifndef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_SENSOR
    #else
        #undef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_FULL
    #endif
#endif

#if defined(ITEAD_SONOFF_RFBRIDGE)
    #ifndef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_RFBRIDGE
    #else
        #undef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_FULL
    #endif
#endif

#if RFM69_SUPPORT == 1
    #ifndef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_RFM69
    #else
        #undef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_FULL
    #endif
#endif

#ifndef WEBUI_IMAGE
    #define WEBUI_IMAGE        WEBUI_IMAGE_SMALL
#endif
