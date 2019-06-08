namespace Espurna {

    namespace Hardware {

        using setup_f = void();

        PROGMEM const char NAME_ESPURNA_CORE[] = "ESPURNA_CORE";
        PROGMEM const char NAME_ITEAD_SONOFF_POW[] = "ITEAD_SONOFF_POW";
        PROGMEM const char NAME_BLITZWOLF_BWSHPX_V23[] = "BLITZWOLF_BWSHPX_V23";

        const size_t size = 3;
        const char* const names[] = {
            NAME_ESPURNA_CORE,
            NAME_ITEAD_SONOFF_POW,
            NAME_BLITZWOLF_BWSHPX_V23
        };

        void setup_ESPURNA_CORE() {
            #include "hardware/ESPURNA_CORE.h"
        }
        void setup_ITEAD_SONOFF_POW() {
            #include "hardware/ITEAD_SONOFF_POW.h"
        }
        void setup_BLITZWOLF_BWSHPX_v23() {
            #include "hardware/BLITZWOLF_BWSHPX_V23.h"
        }

        const setup_f* const setups[] = {
            setup_ESPURNA_CORE,
            setup_ITEAD_SONOFF_POW,
            setup_BLITZWOLF_BWSHPX_v23
        };

    }

}
