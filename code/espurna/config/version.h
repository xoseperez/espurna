// -----------------------------------------------------------------------------
// APP NAME AND VERSION
// -----------------------------------------------------------------------------

#pragma once

#ifndef APP_NAME
#define APP_NAME                "ESPURNA"
#endif

#ifndef APP_VERSION
#define APP_VERSION             "1.22.0"
#endif

#ifndef APP_AUTHOR
#define APP_AUTHOR              "xose.perez@gmail.com"
#endif

#ifndef APP_WEBSITE
#define APP_WEBSITE             "http://tinkerman.cat"
#endif

// Release image name, the dist name of the CI build (espurna-<image>-*): the WebUI installs the files of this image
// from the GitHub releases. Empty in local builds, the WebUI then lists every image
#ifndef ESPURNA_IMAGE
#define ESPURNA_IMAGE           ""
#endif

#ifndef CFG_VERSION
#define CFG_VERSION             17
#endif

