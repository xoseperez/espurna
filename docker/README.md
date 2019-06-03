# Docker build image

This directory contains a docker file for an ESPurna build environment.

Two volumes can be used. 
* `/espurna` with ESPurna source code.
* `/firmware` target directory for complied firmware files.

## Build

```bash
docker build -t espurna-build -f Dockerfile ..
```

## Examples

The simples example will build all firmware files from dev branch to /tmp/firmware.

```bash
docker run --rm -it -v /tmp/firmware/:/firmware espurna-build
```


This example will only build firmware for environment `intermittech-quinled` from local files in `/home/user/espurna`

```bash
docker run --rm -it -v /tmp/firmware/:/firmware -v /home/user/espurna/:/espurna espurna-build intermittech-quinled
```
