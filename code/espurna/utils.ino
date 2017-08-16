/*

UTILS MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

char * ltrim(char * s) {
    char *p = s;
    while ((unsigned char) *p == ' ') ++p;
    return p;
}
