# Over-the-air updates

## Manually driven OTA updates

Once you have flashed your board with the ESPurna firmware you can flash it again over-the-air using PlatformIO and the ```ota``` environment:

```bash
> pio run -t upload -e node-debug-ota
> pio run -t uploadfs -e node-debug-ota
```

When using OTA environment it defaults to the IP address of the device in SoftAP mode. If you want to flash it when connected to your home network best way is to supply the IP of the device:

```bash
> pio run -t upload -e node-debug-ota --upload-port 192.168.1.151
> pio run -t uploadfs -e node-debug-ota --upload-port 192.168.1.151
```

Please note that if you have changed the admin password from the web interface you will have to change it too in the platformio.ini file for the OTA to work. Check for the *upload_flags* option in your ota-enabled environment.

## Automatic OTA updates

You can also use the automatic OTA update feature. Check the [NoFUSS library][6] for more info.

This options is disabled by default. Enable it in your firmware with the -DENABLE_FUSS build flag.


[6]: https://bitbucket.org/xoseperez/nofuss
