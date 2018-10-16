// This file is generate with TZupdate.pl in espurna/code/html
// It uses the content of https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv
// It work with TZ.js (also generated)

typedef struct { 
    char * timeZoneName;
    int16_t  timeZoneOffset;    // offset from GMT 0 in minutes 
    char * timeZoneDSTName;     // NULL if disable
    int16_t  timeZoneDSTOffset; // offset from GMT 0 in minutes 
    uint8_t dstStartMonth;      // start of Summer time if enabled  Month 1 - 12, 0 disabled dst
    uint8_t dstStartWeek;       // start of Summer time if enabled Week 1 - 5: (5 means last)
    uint8_t dstStartDay;        // start of Summer time if enabled Day 1- 7  (1- Sun)
    uint16_t dstStartMin;       // start of Summer time if enabled in minutes
    uint8_t dstEndMonth;        // end of Summer time if enabled  Month 1 - 12
    uint8_t dstEndWeek;         // end of Summer time if enabled Week 1 - 5: (5 means last)
    uint8_t dstEndDay;          // end of Summer time if enabled Day 1-7  (1- Sun)
    uint16_t dstEndMin;         // end of Summer time if enabled in minutes
} TZinfo;

extern TZinfo TZall[];
    