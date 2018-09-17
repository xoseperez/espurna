/* Flash Split for 1M chips, no SPIFFS, 2 sectors for EEPROM */
/* sketch 995KB */
/* eeprom 8KB */
/* reserved 16KB */

MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = 0xf8ff0
}

PROVIDE ( _SPIFFS_start = 0x402FA000 );
PROVIDE ( _SPIFFS_end = 0x402FA000 );
PROVIDE ( _SPIFFS_page = 0x0 );
PROVIDE ( _SPIFFS_block = 0x0 );

INCLUDE "../ld/eagle.app.v6.common.ld"
