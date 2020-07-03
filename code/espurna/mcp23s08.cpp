/*

MCP23S08 MODULE

Copyright (C) 2020 by Eddi De Pieri <eddi at depieri dot com>
Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2016 Plamen Kovandjiev <p.kovandiev@kmpelectronics.eu> & Dimitar Antonov <d.antonov@kmpelectronics.eu>

*/

#include "mcp23s08.h"

#if MCP23S08_SUPPORT

#include <bitset>


#define MCP23S08_CS 15

#define READ_CMD  0x41
#define WRITE_CMD 0x40

#define IODIR   0x00
#define IPOL    0x01
#define GPINTEN 0x02
#define DEFVAL  0x03
#define INTCON  0x04
#define IOCON   0x05
#define GPPU    0x06
#define INTF    0x07
#define INTCAP  0x08
#define GPIO    0x09
#define OLAT    0x0A

/**
 * @brief Relay pins.
 */
const uint8_t RELAY_PINS[DUMMY_RELAY_COUNT] =
{ MCP23S08_REL1PIN, MCP23S08_REL2PIN, MCP23S08_REL3PIN, MCP23S08_REL4PIN };

/**
 * @brief Input pins.
 */
const int OPTOIN_PINS[MCP23S08_OPTOIN_COUNT] =
{ MCP23S08_IN1PIN, MCP23S08_IN2PIN, MCP23S08_IN3PIN, MCP23S08_IN4PIN };

uint8_t  _expTxData[16]  __attribute__((aligned(4)));
uint8_t  _expRxData[16]  __attribute__((aligned(4))); 


McpGpioPin::McpGpioPin(unsigned char pin) :
    BasePin(pin)
{}

inline void McpGpioPin::pinMode(int8_t mode) {
    ::MCP23S08SetDirection(this->pin, mode);
}

inline void McpGpioPin::digitalWrite(int8_t val) {
    ::MCP23S08SetRelayState(this->pin, val);
}

inline int McpGpioPin::digitalRead() {
    return ::MCP23S08GetOptoInState(this->pin);
}

void MCP23S08Setup()
{
    DEBUG_MSG_P(PSTR("[MCP23S08] Initialize SPI bus\n"));
    // Expander settings.
    SPI.begin();
    SPI.setHwCs(MCP23S08_CS);
    SPI.setFrequency(1000000);
    SPI.setDataMode(SPI_MODE0);

    pinMode(MCP23S08_CS, OUTPUT);
    digitalWrite(MCP23S08_CS, HIGH);
    MCP23S08InitGPIO();
}

/**
 * @brief Set a expander MCP23S08 the pin direction.
 *
 * @param pinNumber Pin number for set.
 * @param mode direction mode. 0 - INPUT, 1 - OUTPUT.
 *
 * @return void
 */
void MCP23S08InitGPIO()
{
    // Relays.
    for (uint8_t i = 0; i < DUMMY_RELAY_COUNT; i++)
    {
        DEBUG_MSG_P(PSTR("[MCP23S08] Initialize output GPIO %d\n"), i);
        MCP23S08SetDirection(RELAY_PINS[i], OUTPUT);
    }

}

/**
 * @brief Set a expander MCP23S08 the pin direction.
 *
 * @param pinNumber Pin number for set.
 * @param mode direction mode. 0 - INPUT, 1 - OUTPUT.
 *
 * @return void
 */
void MCP23S08SetDirection(uint8_t pinNumber, uint8_t mode)
{
    uint8_t registerData = MCP23S08ReadRegister(IODIR);

    if (INPUT == mode)
    {
        registerData |= (1 << pinNumber);
    }
    else
    {
        registerData &= ~(1 << pinNumber);
    }

    MCP23S08WriteRegister(IODIR, registerData);
}
 
/**
 * @brief Read an expander MCP23S08 a register.
 *
 * @param address A register address.
 *
 * @return The data from the register.
 */
uint8_t MCP23S08ReadRegister(uint8_t address)
{
    _expTxData[0] = READ_CMD;
    _expTxData[1] = address;

    digitalWrite(MCP23S08_CS, LOW);
    SPI.transferBytes(_expTxData, _expRxData, 3);
    digitalWrite(MCP23S08_CS, HIGH);

    return _expRxData[2];
} 

/**
 * @brief Write data in expander MCP23S08 register.
 *
 * @param address A register address.
 * @param data A byte for write.
 *
 * @return void.
 */
void MCP23S08WriteRegister(uint8_t address, uint8_t data)
{
    _expTxData[0] = WRITE_CMD;
    _expTxData[1] = address;
    _expTxData[2] = data;

    digitalWrite(MCP23S08_CS, LOW);
    SPI.transferBytes(_expTxData, _expRxData, 3);
    digitalWrite(MCP23S08_CS, HIGH);
} 

/**
 * @brief Set expander MCP23S08 pin state.
 *
 * @param pinNumber The number of pin to be set.
 * @param state The pin state, true - 1, false - 0.
 *
 * @return void
 */
void MCP23S08SetPin(uint8_t pinNumber, bool state)
{
    uint8_t registerData = MCP23S08ReadRegister(OLAT);

    if (state)
    {
        registerData |= (1 << pinNumber);
    }
    else
    {
        registerData &= ~(1 << pinNumber);
    }

    MCP23S08WriteRegister(OLAT, registerData);
}

/**
 * @brief Get MCP23S08 pin state.
 *
 * @param pinNumber The number of pin to be get.
 *
 * @return State true - 1, false - 0.
 */
bool MCP23S08GetPin(uint8_t pinNumber)
{
    uint8_t registerData = MCP23S08ReadRegister(GPIO);

    return registerData & (1 << pinNumber);
}
 
/**
 * @brief Set relay new state.
 *
 * @param relayNumber Number of relay from 0 to RELAY_COUNT - 1. 0 - Relay1, 1 - Relay2 ...
 * @param state New state of relay, true - On, false = Off.
 *
 * @return void
 */
void MCP23S08SetRelayState(uint8_t relayNumber, bool state)
{
    // Check if relayNumber is out of range - return.
    if (relayNumber > DUMMY_RELAY_COUNT - 1)
    {
        return;
    }
    
    MCP23S08SetPin(RELAY_PINS[relayNumber], state);
}

/**
 * @brief Get opto in state.
 *
 * @param optoInNumber OptoIn number from 0 to MCP23S08_OPTOIN_COUNT - 1
 *
 * @return bool true - opto in is On, false is Off. If number is out of range - return false.
 */
bool MCP23S08GetOptoInState(uint8_t optoInNumber)
{

    // Check if optoInNumber is out of range - return false.
    if (optoInNumber > MCP23S08_OPTOIN_COUNT - 1)
    {
        return false;
    }

    return !MCP23S08GetPin(optoInNumber);
}

#endif // MCP23S08_SUPPORT
