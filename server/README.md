# ESPurna Update Server

First version of an ESPurna update server, an API that responfs to ESPurna devices with information about the last available firmware.

## Use

1. Modify ```data/versions.js``` with info about available firmware versions depending on current model (device type) and firmware version.
1. Perform GET queries against http://<server>/<model>/<firmware_version>, for instance: ```http://192.168.1.105/espurna/0.9.1```
