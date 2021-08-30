---
name: Bug report
about: Create a report to help us improve
title: ''
labels: ''
assignees: ''

---

*Before creating a new issue please check that you have:*

* *searched the existing [issues](https://github.com/xoseperez/espurna/issues) (both open and closed)*
* *searched the [wiki](https://github.com/xoseperez/espurna/wiki)*
* *asked for help in the [gitter chat](https://gitter.im/tinkerman-cat/espurna) ([also available with any Matrix client!](https://matrix.to/#/#tinkerman-cat_espurna:gitter.im))*
* *done the previous things again :)*

*Fulfilling this template will help developers and contributors to address the issue. Try to be as specific and extensive as possible. If the information provided is not enough the issue will likely be closed.*

*You can now remove this line and the above ones. Text in italic is meant to be replaced by your own words. If any of the sections below are not relevant to the issue (for instance, the screenshots or crash report) then you can delete them.*

**Bug description**
*A clear and concise description of what the bug is.*

**Steps to reproduce**
*Steps to reproduce the behavior.*

**Expected behavior**
*A clear and concise description of what you expected to happen.*

**Screenshots**
*If applicable, add screenshots to help explain your problem.*

**Device information**
*Copy-paste here the information as it is outputted by the device. You can get this information by using the `info` command in the serial terminal, telnet or in the DEBUG tab of WebUI*
*If you cannot get this info from the device, please answer this questions:*
* *Device brand, model and version*
* *Arduino Core version*
* *ESPurna git revision or release version*
* *Flash mode*

**Crash report**
*Save the crash postmortem message reported by the device (or, by using `info` and `crash` commands, if done remotely), which may look something like:*
> Exception (28):
> epc1=0x4021e698 epc2=0x00000000 epc3=0x00000000 excvaddr=0x00000000 depc=0x00000000  
>   
> ctx: sys  
> sp: 3ffffc90 end: 3fffffb0 offset: 01a0  
> \>\>\>stack\>\>\>  
> ...  
> \<\<\<stack\<\<\<  
>  

*And, use one of the following tools to decode it:*
- *https://github.com/mcspr/EspArduinoExceptionDecoder*
- *https://github.com/me-no-dev/EspExceptionDecoder (when using Arduino IDE)*

*When using PlatformIO, it is also possible to use a built-in exception decoder when device's serial connection is available:*
- `pio device monitor --echo -e $environment -f esp8266_exception_decoder`

**Tools used**
* *For a custom builds:*
  * *Operating system*
  * *PlatformIO platform & environment(s) used*
  * *(or) ArduinoIDE version, board and the selected menu options*
* *Browser & version, if the problem is related to the WebUI*

**Additional context**
*Add any other information about the problem here.*
