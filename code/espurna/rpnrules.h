/*

RPN RULES MODULE
Use RPNLib library (https://github.com/xoseperez/rpnlib)
Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

// TODO: need this prototype for .ino
struct rpn_context;

#if RPN_RULES_SUPPORT

#include <rpnlib.h>

void rpnSetup();

#endif
