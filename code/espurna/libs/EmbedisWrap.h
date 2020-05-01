// -----------------------------------------------------------------------------
// Wrap class around Embedis (settings & terminal)
// -----------------------------------------------------------------------------

#pragma once

#include <Embedis.h>

class EmbedisWrap : public Embedis {

    public:

        EmbedisWrap(Stream& stream, size_t buflen = 128, size_t argvlen = 8) :
            Embedis(stream, buflen, argvlen)
        {}

        unsigned char getCommandCount() {
            return commands.size();
        }

        String getCommandName(unsigned int i) {
            if (i < commands.size()) return commands[i].name;
            return String();
        }

};
