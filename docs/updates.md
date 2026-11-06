# Firmware updates and releases

How this fork builds its images, where the WebUI finds them and how each chip installs an update. The image list and
the first flash are in the [README](../README.md#download); the Beken details in [beken/README.md](../beken/README.md).

## Releases

Every push to `esp32-port` (and every `esp32-*` tag, or a manual run) is built by
[`.github/workflows/esp32.yml`](../.github/workflows/esp32.yml) and becomes **one release** with the images of all chips.
Its tag is `esp32-YYMMDD-<sha7>` (the chip prefix is historical, the release holds every chip). The newest 5 automatic
releases are kept, older ones are deleted with their tags; releases of hand-made `esp32-*` tags stay.

| Build (matrix) | Release files | Image name (`ESPURNA_IMAGE`) |
|----------------|---------------|------------------------------|
| `esp32-generic` of `platformio_esp32.ini` | `espurna-esp32-matter-full.bin`, `-ota.bin`, `espurna-esp32-safeboot.bin` | `esp32-matter` |
| `esp32-nspanel` | `espurna-esp32-matter-nspanel-full.bin`, `-ota.bin` | `esp32-matter-nspanel` |
| `beken-bk7231n` of `platformio_beken.ini` | `espurna-beken-matter.bin`, `espurna-beken-initial-tuya.uf2`, `espurna-beken-initial-quectel.uf2` | `beken-matter` |
| `esp8266-1m`, `-1m-light`, `-1m-sensor`, `-4m` of `platformio.ini` | `espurna-<image>.bin`, `.bin.gz` | the same as the build |
| `code/mini-app` | `espurna-esp8266-1m-mini.bin`, `.bin.gz` | `esp8266-1m-mini` |

The firmware knows the image it was built as (`ESPURNA_IMAGE`, `fwImage` in the WebUI) and its version carries the git
revision (`1.22.0-git<sha>`; on Beken through `scripts/beken_version.py`). The WebUI finds the running release by that
revision and lists only the files of the running image.

### firmware.json on GitHub Pages

The `pages` job copies the kept releases to GitHub Pages and writes
[`firmware.json`](https://igorufox.github.io/espurna/firmware.json): per release the tag, date, commit and, per image,
its files with kind (`ota`, `serial`, `safeboot`), size and SHA-256. The release assets themselves cannot be used by the
update code: GitHub serves them without CORS (the browser may not read them) and only with ECDHE ciphers (the Beken
mbedTLS has none). Pages sends `Access-Control-Allow-Origin: *`.

Another list: the `webFwUrl` setting (*ADMIN*, OTA, *Release list*); empty is the default URL above. The ESP32 safeboot and
the ESP8266 mini read the same setting from the ESPurna settings.

### CI notes

- **Python 3.13**, not `3.x`: 3.15 broke the build (pioarduino takes 3.10 to 3.14, LibreTiny could not make its venv).
  The PlatformIO cache key has `py313` in it, the caches of the 3.15 runs hold a broken LibreTiny venv.
- PlatformIO Core 6.2 or newer (pioarduino needs it).
- ESP32: `pio run -d safeboot-app` runs first, `scripts/esp32_merge.py` puts the safeboot into the factory partition of the
  `-full.bin`.
- The mini `.bin.gz` is made with `pigz -11` (zopfli), about 4 KB smaller than `gzip -9`: it has to fit behind the
  biggest 1 MB image. The other `.bin.gz` files are `gzip -9 -n`.
- The Pages artifact is named per run attempt (`github-pages-<attempt>`): a re-run otherwise finds the artifact of the
  failed attempt as well and `deploy-pages` stops.
- Beken: the Matter library is not built here. It is a release asset of
  [igorufox/matter-bk7231n](https://github.com/igorufox/matter-bk7231n), see [below](#beken-matter-library).

## Updating from the WebUI

*ADMIN*, OTA section:

- **Upgrade** - a file from this computer (*Browse*, *Upgrade*), as before.
- **From GitHub**, *Show releases* - the kept releases with the files of this board's image. *Install* downloads the file
  into the browser (or lets the board download it, ESP32) and lists the steps ahead with the step in progress. The board
  restarts on its own during an install: keep it powered and keep the page open until the last step.

What happens after *Install* depends on the chip.

### ESP32: through safeboot

The ESP32 images use the safeboot flash layout ([`code/esp32_4mb_safeboot.csv`](../code/esp32_4mb_safeboot.csv), the offsets
of Tasmota): the ESPurna safeboot ([`code/safeboot-app`](../code/safeboot-app), plain ESP-IDF) in the factory partition at
`0x10000` (832 KB), one 2880 KB app slot for ESPurna at `0xE0000`. ESPurna cannot overwrite itself, so `/upgrade` answers
with an error and every update goes through safeboot:

1. The WebUI asks `/safeboot/status` for the flash layout and the SHA-256 of the installed safeboot.
2. When the release's `espurna-esp32-safeboot.bin` differs (SHA-256), the browser downloads it and uploads it to
   `/safeboot/install`; the SHA-256 is checked again after the write.
3. `/safeboot/update` with the URL and SHA-256 of the `-ota.bin` (from `firmware.json`): ESPurna stores them in NVS
   (`safeboot`/`url`, `sha256`, `image`) and restarts into safeboot.
4. Safeboot joins Wi-Fi with the ESPurna settings (`ssid0`/`pass0`, `ssid1`/`pass1`, static IP, hostname), downloads the
   `-ota.bin` over https (ESP-IDF certificate bundle), up to 3 attempts, checks the SHA-256, writes app0 and boots
   ESPurna. The result goes to NVS `safeboot`/`result`, the WebUI shows it.

**Two-slot boards** (older ESP32 builds, `min_spiffs.csv` / `default.csv`): the Matter images no longer fit their slots.
The WebUI migrates them over Wi-Fi: the browser uploads the safeboot image to `/safeboot/migrate` together with the
`-ota.bin` URL; ESPurna writes the safeboot over the first slot, replaces the partition table and erases otadata, the
board restarts into safeboot, which downloads ESPurna into the new app0. NVS keeps its place, so the settings stay.
ESPurna has to run from the second slot for that; on the first slot it asks for one update more first.

**Safeboot itself**: web page on port 80 (user `admin`, the ESPurna admin password): the releases of the running image
(from `firmware.json`, read by the browser), an upload of an `-ota.bin` into app0, a download from a URL. Without Wi-Fi in
30 s it opens its own access point `<hostname>-SAFEBOOT` (WPA2, admin password) at 192.168.4.1. The next boot is set back
to app0 right at its start, so a crash or a power cut in safeboot returns to ESPurna; after 15 minutes without activity it
boots ESPurna as well. Other ways in: the `safeboot` terminal command, the `/safeboot` page of ESPurna, and the crash-loop
guard (5 crashes in a row in the first minute after boot switch to safeboot, 3 on the NSPanel). ESPurna switches only
into the ESPurna safeboot: a factory partition with anything else (Tasmota's safeboot, nothing) is refused.

NSPanel: the same module and the same steps as every ESP32 (`esp32-matter-nspanel` gets the release's one
`espurna-esp32-safeboot.bin` like the other ESP32 images). By what the panel has:
- flashed with the full image (ESPurna safeboot in the factory partition): step 3 only, step 2 when the release brings a
  newer safeboot;
- installed over Tasmota (Tasmota's safeboot in the factory partition, same offsets): `/safeboot/status` says
  `safeboot` with Tasmota's image, its SHA-256 differs, so step 2 replaces it with ours before step 3;
- older ESPurna on the two-slot layout (`min_spiffs.csv`): the migration above. The file system (LittleFS with
  `nspanel.json`, 128 KB at `0x3D0000`) lies behind the new app0 and keeps its place in the new table, so the screen
  configuration stays. Panels built before this change have no `/safeboot` at all and their slot is 1.9 MB: once,
  upload an NSPanel image without Matter (1.91 MB, `MATTER_SUPPORT=0`) with *Upgrade*, twice when the panel runs from
  its first slot, then the release list migrates it.

### Beken: the browser uploads, the board picks its part

The board cannot download from GitHub (no ECDHE in its mbedTLS), so the browser downloads `espurna-beken-matter.bin` from
Pages and uploads it to `/upgrade`, the same as *Upgrade* with a file.

That file is a container (`ESPBEKEN`, 64-byte header with the build id, a table of parts): the RBL for the Tuya bootloader
(the image without its Matter code), the RBL for the Quectel bootloader (the whole image) and the Matter code. The board
knows its bootloader from the CRC32 of the first `0x11000` bytes of flash (Tuya `f684a1dc`, Quectel `64b02979`), writes
only the part for it into that bootloader's `download` partition, and the bootloader installs it at the next boot. An
unknown bootloader gets no OTA at all. The first bytes of the RBL are written last, so an interrupted upload leaves nothing
for the bootloader to install.

- **Quectel bootloader, 4 MiB**: one upload, one restart.
- **Tuya bootloader, 2 MiB** (CB2S, CB3S, CBU ...): 1124 KiB for the firmware, 664 KiB for an update, ESPurna with Matter
  is about 1.4 MiB. The Matter code lives in the update space. So the first upload writes the image, the board answers
  `OK, again` and restarts without Matter; the WebUI uploads the same file once more and the board, now running that build,
  writes its Matter code.
- **Matter running on the Tuya layout**: its code sits where the update goes. The board answers HTTP 409, restarts without
  Matter for one boot (`matterSkip`), and the WebUI uploads again.

The WebUI does all of that by itself: up to seven steps and three restarts. An install that stops half-way leaves the
board on its firmware (at worst without Matter, which the MATTER page then says: *Install the Matter code* there, or
upload the same file once more). A plain LibreTiny `.uf2` is still accepted when it was built for the board's bootloader.

### ESP8266: upload, or the mini on 1 MB boards

The ESP8266 writes an update behind the running image and copies it over at the next boot (eboot unpacks `.bin.gz`). What
fits is the free space the board reports minus one sector:

- **Fits** (4 MB layout, small images): the browser downloads the file and uploads it to `/upgrade`, one restart.
- **1 MB boards**: a full image leaves about 200 KB, too little for the next one. Two steps through the mini
  ([`code/mini-app`](../code/mini-app)):
  1. `POST /ota/fetch` stores the URL and SHA-256 of the full `.bin.gz` in the settings (`otaFetchUrl`, `otaFetchSha256`).
  2. The browser uploads `espurna-esp8266-1m-mini.bin.gz` to `/upgrade`, the board restarts into the mini.
  3. The mini reads the ESPurna settings in place (Wi-Fi, static IP, hostname, `adminPass`, `webFwUrl`), takes the stored
     request once, downloads the `.bin.gz` (https), checks the SHA-256, writes it and restarts into the full image.
  4. The full image shows the result (`/ota/status`, `otaFetchResult`).

The mini keeps its own page on port 80 after a failure, or when it has no request: status, last result, the releases
with the 1 MB images that fit, a file upload. Without Wi-Fi in 30 s it opens `<hostname>-MINI` (WPA2, admin password) at
192.168.4.1, and tries the Wi-Fi again every 5 minutes. Its log is on the TX pin at 115200.

The full images do not download anything themselves (`OTA_FETCH_SUPPORT` is 0): BearSSL needs about 27 KB of heap,
which a full image never has. Older firmware without `/ota/status` can only take a file that fits; on a 1 MB board upload
`espurna-esp8266-1m-mini.bin.gz` with *Upgrade* and pick the image on the mini page.

**From a board build to a generic image.** The generic images take the pins from the settings. Apply the board's
template (*ADMIN*, *Device Template*) before the update (it writes the values the board build had, nothing changes) or
right after the first boot.

## TLS and integrity

| Who downloads | TLS | Integrity |
|---------------|-----|-----------|
| Browser (all chips) | validated https to GitHub Pages | SHA-256 of `firmware.json` |
| ESP32 safeboot | ESP-IDF certificate bundle | SHA-256 from `firmware.json`, checked before ESPurna boots it |
| ESP8266 mini | BearSSL, TLS 1.2, RSA key exchange + AES-128-GCM, **no certificate check** (no CA store, no clock) | SHA-256 required, taken by the browser from `firmware.json` over validated https |
| Beken | none, the browser uploads | the container's part CRC32 and build id |

## Beken Matter library

`libMatterApp.a` (connectedhomeip for the BK7231N) is built in [igorufox/matter-bk7231n](https://github.com/igorufox/matter-bk7231n)
by its own GitHub Actions workflow (`release.yml`): `tools/ci_build.sh` on Ubuntu 24.04 with the Linux build of the gcc
LibreTiny uses (`toolchain-gccarmnoneeabi@1.100301.220327`, GCC 10.3.1) and pinned connectedhomeip, LibreTiny 1.14.0 and
BDK packages, after every push to `main` that changes `gn/`, `lwip-override/`, `tools/` or the workflow. Each build is a
release `libmatter-<yyyymmdd>-<sha7>` with `matter-bk7231n-lib.zip` (`lib/libMatterApp.a` + `include/lwipopts.h`) and its
`.sha256`; the newest 5 are kept.

`custom_matter_lib = latest` in `platformio_beken.ini` ([`scripts/beken_matter_lib.py`](../code/scripts/beken_matter_lib.py))
takes the newest of them through the GitHub API (`GITHUB_TOKEN` when set), checks the zip against the `.sha256` asset and
unpacks it into `.pio/matter-lib/<hash>`; without network the last one is used again. A fixed library:
`custom_matter_lib = <zip URL>` with `custom_matter_lib_sha256`; a local build: `ESPURNA_MATTER_LIB_DIR=<folder>`.
lwIP of the firmware is built with the library's `lwipopts.h`.

## Known limits

- **1 MB margin.** The mini `.bin.gz` has to fit behind the biggest 1 MB image (`esp8266-1m-light`, light + IR), and every
  full `.bin.gz` behind the mini. Every KB added to a 1 MB image or to the mini counts.
- **esp8266-4m RAM.** Every driver costs static RAM even when idle. With all of them the 4 MB image had about 4 KB of heap
  left and crashed during an update; it now has relays, buttons, LEDs, RF, MCP23S08 and the sensor drivers only (about
  15 KB free after 5 minutes on an ESP-12F).
- **No board-side download on ESP8266 full images**, see above.
- **Mini TLS** does not validate the certificate; the SHA-256 from `firmware.json` is what protects the image.
- **Boards that need two 1 MB images**: the Smartlife Mini Smart Socket (light + power meter) and the MagicHome ZJ-WFMN-B11
  (light + 433 MHz receiver) run `esp8266-1m-light`; their sensor / RF part stays off.
- **NSPanel**: panels with ESPurna older than the shared safeboot module need one manual *Upgrade* with a no-Matter
  image first (see above). Keeping the file system through the migration is not tested on hardware yet.
