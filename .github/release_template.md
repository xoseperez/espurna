# IMPORTANT NOTICE

When updating from 1.14.1 make sure **there is enough available space on the device** (usually, "available space" rounded to the nearest 4096 byte increment) to perform the OTA upgrade. Telnet and direct HTTP upgrades with an .bin too big **will result in a bricked device**. espota.py and Web Interface upgrades will issue a warning.

Make sure to **only** perform OTA upgrade with a properly powered device. Attempting to flash and / or use a normally AC powered device (like a Sonoff) instead powered through 3v3 **may** result in unexpected issues with the firmware.

# Snapshot build

**How to upgrade "over-the-air" aka OTA**: https://github.com/xoseperez/espurna/wiki/OTA
**Latest changes**: https://github.com/xoseperez/espurna/compare/$last_tag...$tag
**CHANGELOG.md**: https://github.com/xoseperez/espurna/blob/$tag/CHANGELOG.md
