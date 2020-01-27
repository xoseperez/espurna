/*
Software small timer.
Internal version 3.6   [ heX ] 2019
Repository: https://github.com/heX16/small_timer
*/
#ifndef TIMER_HEX_LIB
#define TIMER_HEX_LIB


#ifndef TIMER_GET_TIME
  #ifdef Arduino_h
    // define for Arduino library
    #define TIMER_GET_TIME millis()
  #endif // Arduino_h

  #ifdef WINAPI
    #define TIMER_GET_TIME GetTickCount()
  #endif // WINAPI

  //todo: check ESP chipset
  //todo: add support - linux
  //todo: add support? - native ATmega chipset
  // for linux see: https://stackoverflow.com/questions/7729686/convert-gettickcount-vxworks-to-linux
#endif // TIMER_GET_TIME

#define TIMER_GETBIT(x,bit)  ((x & (1ULL << (bit))) != 0)
#define TIMER_SETBIT1(x,bit) {(x) |= (1ULL << (bit));}
#define TIMER_SETBIT0(x,bit) {(x) &= ~(1ULL << (bit));}

// timer template
template <
    typename TTimer, // type Timer
    unsigned long MaxValue, // max time in Timer
    unsigned long BitMask, // mask of value
    unsigned int DisableBit, // timer disable bit number
    unsigned int UserFlagBit, // user flag bit number - In some implementations, you can use the custom flag
    unsigned int PrecDiv, // precision divider
    unsigned long InitValue // disabled timer value
>
class tpTimer {
public:

  typedef TTimer DataType;

  // declare data - it's all data for this class
  TTimer timer;

  tpTimer() {
    init();
  }

  // Start timer. If the timer is already running, the timer restarts.
  inline void start(const TTimer time) {
    // Note: this condition should be minimized by compiler optimization (but this optimizations is disabled during debugging mode)
    if (UserFlagBit == 0) {
      // "user flag" not using
      timer = (((TTimer)TIMER_GET_TIME / PrecDiv) + time) & BitMask;
    } else {
      // safe "user flag"
      bool f = getFlag();
      timer = (((TTimer)TIMER_GET_TIME / PrecDiv) + time) & BitMask;
      setFlag(f);
    }
  }

  // Start timer. If the timer is already running then restarting does not occur.
  bool startOnce(const TTimer time) {
    if ( ! enabled()) {
      start(time);
      return true;
    } else
      return false;
  }

  // Timer stop.
  inline void stop() {
    TIMER_SETBIT1(timer, DisableBit);
  }

  // Init timer.
  inline void init() {
    timer = InitValue;
  }

  // Read the status of the timer. If started, returns True.
  inline bool enabled() {
    return ! TIMER_GETBIT(timer, DisableBit);
  }

  // Main function. Must be called in every cycle. If the timer has worked, then True is returned once.
  bool run() {
    if ( ! enabled())
      return false;
    //TTimer temp = ((TTimer)((TTimer)TIMER_GET_TIME - timer) & 0x7FFF);
    if ( ((TTimer)((TTimer)(TIMER_GET_TIME / PrecDiv) - timer) & BitMask) < MaxValue) {
      stop();
      return true;
    } else
      return false;
  }

  // Read user flag bit. If there is no flag support then False is always returned.
  // Some timer classes may contain a custom flag that the user use at his discretion.
  bool getFlag() {
    // Note: this condition should be minimized by compiler optimization
    if (UserFlagBit == 0)
      return false;
    else
      return TIMER_GETBIT(timer, UserFlagBit);
  }

  // Write to user flag bit.
  void setFlag(bool newValue) {
    // Note: this condition should be minimized by compiler optimization
    if (UserFlagBit == 0)
      return;
    else
      if (newValue)
        TIMER_SETBIT1(timer, UserFlagBit)
      else
        TIMER_SETBIT0(timer, UserFlagBit);
  }

};

//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

template <
    typename TTimer, // type Timer
    unsigned long MaxValue, // max time in Timer
    unsigned long BitMask, // mask of value
    unsigned int DisableBit, // timer disable bit number
    unsigned int UserFlagBit, // user flag bit number - In some implementations, you can use the custom flag
    unsigned int PrecDiv, // precision divider
    unsigned long InitValue // disabled timer value
>
class tpTimerExternal: public tpTimer<TTimer, MaxValue, BitMask, DisableBit, UserFlagBit, PrecDiv, InitValue> {
public:

  inline void start(const TTimer time, TTimer externalTime) {
    this->timer = (((TTimer)externalTime / PrecDiv) + time) & BitMask;
  }

  bool startOnce(const TTimer time, TTimer externalTime) {
    if ( ! (this->enabled())) {
      this->start(time, externalTime);
      return true;
    } else
      return false;
  }

  bool run(TTimer externalTime) {
    if ( ! (this->enabled()))
      return false;

    if ( ((TTimer)((TTimer)(externalTime / PrecDiv) - (this->timer)) & BitMask) < MaxValue) {
      this->stop();
      return true;
    } else
      return false;
  }

};

//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

template <
    class T,
    unsigned long DefaultTime
>
class tpTimerDefaultValue : public T {
  inline void start() { T::start(DefaultTime); }
  inline bool startOnce() { return T::startOnce(DefaultTime); }
};

//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

// Timer config examples:

/*
    typename TTimer, // type Timer
    int MaxValue, // max time in Timer
    int BitMask, // mask of value
    int DisableBit, // timer disable bit number
    int UserFlagBit, // user flag bit number - In some implementations, you can use the custom flag
    int PrecDiv // precision divider
    int InitValue // disabled timer value
*/

// max 64 ms.
typedef tpTimer<uint8_t, 63, 0x7F, 7, 0, 1, 0xFF>
    csTimer8bit_64ms;

// max 128 ms. div 4. jitter - 4 ms! And support user flag.
typedef tpTimer<uint8_t, 31, 0x3F, 6, 7, 4, 0xFF>
    csTimer8bit_128ms_J4ms_Flag;

// max 64 second. div 256. jitter - 0.25 second! Recomended minimal - 10 second.  Size - 1 byte.
typedef tpTimer<uint8_t, 63, 0x7F, 7, 0, 256, 0xFF>
    csTimer8bit_64sec_J256ms;

// max 1 second. div 8. jitter - 8 ms. Size - 1 byte.
typedef tpTimer<uint8_t, 63, 0x7F, 7, 0, 8, 0xFF>
    csTimer8bit_1sec_J8ms;

// max 16 second.
typedef tpTimer<uint16_t, 16383, 0x7FFF, 15, 0, 1, 0xFFFF>
    csTimer16bit_16sec;

// max 8 second. And support user flag.
typedef tpTimer<uint16_t, 8191, 0x3FFF, 14, 15, 1, 0xFFFF>
    csTimer16bit_16sec_Flag;

// max 16 min. div 64. jitter - 64 ms
typedef tpTimer<uint16_t, 16383, 0x7FFF, 15, 0, 64, 0xFFFF>
    csTimer16bit_16min_J64ms;

// max 279 min (4,5 hour). div 1024. jitter - 1 second.
typedef tpTimer<uint16_t, 16383, 0x7FFF, 15, 0, 64, 0xFFFF>
    csTimer16bit_4hour_J1sec;

// max 12,4 day.
typedef tpTimer<uint32_t, 1073741823, (unsigned long)0x7FFFFFFF, 31, 0, 1, (unsigned long)0xFFFFFFFF>
    csTimer32bit_12day;

// max 6 day. And support user flag.
typedef tpTimer<uint32_t, 536870911, (unsigned long)0x3FFFFFFF, 30, 31, 1, (unsigned long)0xFFFFFFFF>
    csTimer32bit_6day_Flag;

// default timer:

typedef csTimer32bit_12day csTimer; // big timer, max length - 12 day. Size - 4 byte.

typedef csTimer16bit_16sec csTimerShort; // short time, max length - 16 second. Size - 2 byte.



//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

// default timer with "default time" support

template <unsigned long DefaultTime>
class csTimerDef : public csTimer {
public:
  using csTimer::start; // - function overload in class
  using csTimer::startOnce;

  inline void start() { csTimer::start(DefaultTime); }
  inline bool startOnce() { return csTimer::startOnce(DefaultTime); }
};

template <unsigned long DefaultTime>
class csTimerShortDef : public csTimerShort {
public:
  using csTimerShort::start;
  using csTimerShort::startOnce;

  inline void start() { csTimerShort::start(DefaultTime); }
  inline bool startOnce() { return csTimerShort::startOnce(DefaultTime); }
};

//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

// any timer with "default time" support

template <class T, unsigned long DefaultTime>
class csTimerSetDefault : public T {
public:
  using typename T::DataType::start;
  using typename T::DataType::startOnce;

  inline void start() { T::start(DefaultTime); }
  inline bool startOnce() { return T::startOnce(DefaultTime); }
};

// example: typedef csTimerSetDefault <csTimer, 1000> csTimer;


#endif // TIMER_HEX_LIB
