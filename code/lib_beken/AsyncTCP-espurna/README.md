# AsyncTCP
![Build Status](https://github.com/esphome/AsyncTCP/actions/workflows/push.yml/badge.svg)
A fork of the [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) library by [@me-no-dev](https://github.com/me-no-dev) for [ESPHome](https://esphome.io).

### Async TCP Library for ESP32 Arduino

This is a fully asynchronous TCP library, aimed at enabling trouble-free, multi-connection network environment for Espressif's ESP32 MCUs.

This library is the base for [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)

## AsyncClient and AsyncServer
The base classes on which everything else is built. They expose all possible scenarios, but are really raw and require more skills to use.
