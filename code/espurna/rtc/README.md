# RTC support module

## commands in debug terminal
   - RTC return current rtc time
   - RTC YYYY-MM-DD DoW HH:MM:SS setup rtc

## defines to enable support for modules in hardware.h
   - #define RTC_SUPPORT              0/1 enable/disable RTC support
   - #define RTC_PROVIDER             RTC_PROVIDER_DS3231 etc... spec which rtc ic to use
   - #define RTC_RECOVERY_CNT         10(def) // 0 no attempt to switch back to NTP time
   - #define RTC_NTP_SYNC_ENA         0/1      // 0 independed rtc, not synced with ntp,
                                                  1 rtc synced with ntp on ntp success
## to add new rtc ic support
   - see ds3231.h as example, create your_ic.h file and write two
     functions time_t getTime_rtc() and uint8_t setTime_rtc(time_t nt)
   - define your provider constant in config/types.h
   - add your provider section in config/rtc.h
