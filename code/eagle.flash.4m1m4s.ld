/* Flash Split for 4M chips, ~1M for SPIFFS, 4 sectors for EEPROM */
/* sketch 1019KB */
/* empty/ota? 2048KB */
/* spiffs 992KB */
/* eeprom 16KB */
/* reserved 16KB */

MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = 0xfeff0
}

PROVIDE ( _SPIFFS_start = 0x40500000 );
PROVIDE ( _SPIFFS_end = 0x405F8000 );
PROVIDE ( _SPIFFS_page = 0x100 );
PROVIDE ( _SPIFFS_block = 0x2000 );

INCLUDE "../ld/eagle.app.v6.common.ld"
