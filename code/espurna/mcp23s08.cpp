/*

MCP23S08 MODULE

Copyright (C) 2020 by Eddi De Pieri <eddi at depieri dot com>

Adapted from https://github.com/kmpelectronics/Arduino
Copyright (C) 2016 Plamen Kovandjiev <p.kovandiev@kmpelectronics.eu> & Dimitar Antonov <d.antonov@kmpelectronics.eu>

(ref. https://github.com/kmpelectronics/Arduino/blob/master/ProDinoWiFiEsp/src/PRODINoESP8266/src/KMPDinoWiFiESP.cpp)

*/

#include "mcp23s08.h"

#if MCP23S08_SUPPORT

#include "mcp23s08_pin.h"

#include <SPI.h>
#include <bitset>

// TODO: check if this needed for SPI operation
#define MCP23S08_CS_PIN 15

// Known commands
#define READ_CMD  0x41
#define WRITE_CMD 0x40

// Registers
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

static uint8_t  _mcp23s08TxData[16]  __attribute__((aligned(4)));
static uint8_t  _mcp23s08RxData[16]  __attribute__((aligned(4)));

namespace {

class GpioMcp23s08 : public GpioBase {
public:
    constexpr static size_t Pins { 8ul };

    using Pin = McpGpioPin;
    using Mask = std::bitset<Pins>;

    const char* id() const {
        return "mcp23s08";
    }

    size_t pins() const {
        return Pins;
    }

    bool lock(unsigned char index) const override {
        return _lock[index];
    }

    void lock(unsigned char index, bool value) override {
        _lock.set(index, value);
    }

    bool valid(unsigned char index) const override {
        return (index < Pins);
    }

    BasePinPtr pin(unsigned char index) override {
        return std::make_unique<McpGpioPin>(index);
    }

private:
    Mask _lock;
};

} // namespace

void MCP23S08Setup()
{
    DEBUG_MSG_P(PSTR("[MCP23S08] Initialize SPI bus\n"));

    // Expander SPI settings
    SPI.begin();
    SPI.setHwCs(true);
    SPI.setFrequency(1000000);
    SPI.setDataMode(SPI_MODE0);

    pinMode(MCP23S08_CS_PIN, OUTPUT);
    digitalWrite(MCP23S08_CS_PIN, HIGH);
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
    _mcp23s08TxData[0] = READ_CMD;
    _mcp23s08TxData[1] = address;

    digitalWrite(MCP23S08_CS_PIN, LOW);
    SPI.transferBytes(_mcp23s08TxData, _mcp23s08RxData, 3);
    digitalWrite(MCP23S08_CS_PIN, HIGH);

    return _mcp23s08RxData[2];
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
    _mcp23s08TxData[0] = WRITE_CMD;
    _mcp23s08TxData[1] = address;
    _mcp23s08TxData[2] = data;

    digitalWrite(MCP23S08_CS_PIN, LOW);
    SPI.transferBytes(_mcp23s08TxData, _mcp23s08RxData, 3);
    digitalWrite(MCP23S08_CS_PIN, HIGH);
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
 * @param pinNumber The number of pin to get.
 *
 * @return State true - 1, false - 0.
 */
bool MCP23S08GetPin(uint8_t pinNumber)
{
    uint8_t registerData = MCP23S08ReadRegister(GPIO);

    return registerData & (1 << pinNumber);
}

/**
 * @brief Ensure pin number is valid.
 *
 * @param pinNumber The number of pin to be get.
 *
 * @return State true - 1, false - 0.
 */
bool mcpGpioValid(unsigned char gpio)
{
    return gpio < McpGpioPins;
}

GpioBase& mcp23s08Gpio() {
    static GpioMcp23s08 gpio;
    return gpio;
}

#endif // MCP23S08_SUPPORT
