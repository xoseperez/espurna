/*
sketch: 1019KB
fs: 992KB
eeprom: 16KB
*/

MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = 0xfeff0
}

/*
Provide both _SPIFFS_ and _FS_ to be compatible with 2.3.0...2.6.0+ and
any library that is using old _SPIFFS_...
*/

PROVIDE ( _SPIFFS_start = 0x40300000 );
PROVIDE ( _SPIFFS_end = 0x403f8000 );
PROVIDE ( _SPIFFS_page = 0x100 );
PROVIDE ( _SPIFFS_block = 0x2000 );

PROVIDE ( _FS_start = _SPIFFS_start );
PROVIDE ( _FS_end = _SPIFFS_end );
PROVIDE ( _FS_page = _SPIFFS_page );
PROVIDE ( _FS_block = _SPIFFS_block );

INCLUDE "local.eagle.app.v6.common.ld"
