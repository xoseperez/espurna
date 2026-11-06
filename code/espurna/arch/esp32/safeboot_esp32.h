/*

SAFEBOOT (ESP32)

Asymmetric flash layout (esp32_4mb_safeboot.csv, Tasmota's offsets): a factory app with the ESPurna safeboot image
(code/safeboot-app, reads Wi-Fi and the admin password from the ESPurna settings) and a single big OTA app for ESPurna:
- `safeboot` terminal command: next boot from the factory partition, where the new image is uploaded
- crash-loop guard: a few crashes in a row switch to safeboot, so a broken image can still be replaced over Wi-Fi
- web /upgrade answers with an error when there is no second OTA slot (Update would overwrite the running image)
- web /safeboot: migration from the two-slot layouts over Wi-Fi, update from an URL or a file through safeboot,
  install / replace the safeboot image
- NVS / NVS.ERASE terminal commands: what fills the 20 KB NVS partition

*/

#pragma once

// Early, right after the basic modules (works in the 'unstable' system mode as well)
void safebootSetup();

// The /safeboot page (flash layout migration, update from an URL) and the /upgrade guard, after webSetup()
void safebootWebSetup();
