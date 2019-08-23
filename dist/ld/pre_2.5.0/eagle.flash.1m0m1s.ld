/*
sketch: 999KB
fs: 0KB
eeprom: 4KB
*/

MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = 0xf9ff0
}

/*
Provide both _SPIFFS_ and _FS_ to be compatible with 2.3.0...2.6.0+ and
any library that is using old _SPIFFS_...
*/

PROVIDE ( _SPIFFS_start = 0x402fb000 );
PROVIDE ( _SPIFFS_end = 0x402fb000 );
PROVIDE ( _SPIFFS_page = 0x0 );
PROVIDE ( _SPIFFS_block = 0x0 );

PROVIDE ( _FS_start = _SPIFFS_start );
PROVIDE ( _FS_end = _SPIFFS_end );
PROVIDE ( _FS_page = _SPIFFS_page );
PROVIDE ( _FS_block = _SPIFFS_block );

INCLUDE "eagle.app.v6.common.ld"
