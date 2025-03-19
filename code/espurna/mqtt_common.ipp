/*

Part of the MQTT MODULE

Copyright (C) 2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/
#pragma once

#include "types.h"

#include <algorithm>

namespace espurna {
namespace mqtt {

// Currently, nothing is exported. In case external API becomes lacking, make sure to remove this ns and update headers
namespace {

// MQTT v3.1.1 topic filter string
// - every topic level allows every utf-8 character besides '\0' aka null
// - allow '/' between levels
// - allow '+' within levels as a standalone character
// - allow '#' within a level, but only when that level is
//   either the first one in the topic or the very last one
bool is_valid_topic_filter(espurna::StringView topic) {
    if (!topic.length()) {
        return false;
    }

    enum class State {
        Init,
        Hash,
        Path,
        Plus,
        Separator,
    };

    bool ok { true };
    State state { State::Init };

    for (auto it = topic.begin(); it != topic.end(); ++it) {
        if (state == State::Hash) {
            ok = false;
            break;
        }

        switch (*it) {
        case '\0':
            ok = false;
            break;

        case '/':
            state = State::Separator;
            break;

        case '+':
            if ((state != State::Separator)
             && (state != State::Init))
            {
                ok = false;
                break;
            }

            state = State::Plus;
            break;

        case '#':
            if ((state != State::Separator)
             && (state != State::Init))
            {
                ok = false;
                break;
            }

            state = State::Hash;
            break;

        default:
            if ((state != State::Separator)
             && (state != State::Init)
             && (state != State::Path))
            {
                ok = false;
                break;
            }

            if (state == State::Separator) {
                state = State::Init;
                break;
            }

            state = State::Path;
            break;
        }

        if (!ok) {
            break;
        }
    }

    return ok;
}

// MQTT v3.1.1 topic string
// - every topic level allows every utf-8 character besides '\0' aka null
// - allow '/' between levels
// - do not allow '+' or '#' wildcards to appear
bool is_valid_topic(espurna::StringView topic) {
    if (!topic.length()) {
        return false;
    }

    for (auto it = topic.begin(); it != topic.end(); ++it) {
        switch (*it) {
        case '/':
            break;

        case '#':
        case '+':
        case '\0':
            return false;
        }
    }

    return true;
}

// Root topic differs from a normal MQTT topic and is specific to this app.
// - '+' wildcard is not allowed, this can only the a part of the later constructed topic
//   where '#' is replaced with either '+' or a multi-level inner topic aka magnitude
// - '#' wildcard allowed anywhere in the topic, not just at the end.
//   similarly to '+', imply it captures any number of levels in-between
// - '#' wildcard implies there are at least one or more levels
//   value within is expected to be present (checked when matching the filter)
//
// '#' MUST to be present in the topic, but exactly once. Usual restrictions still apply.
// - '#' allowed
// - '#/...' allowed
// - '.../#/...' allowed
// - '.../#' allowed
// - '#foo' or 'f#oo' or 'foo#' are not allowed
// - '.../#/.../#' is not allowed
bool is_valid_root_topic(espurna::StringView topic) {
    if (!topic.length()) {
        return false;
    }

    enum class State {
        Init,
        Separator,
        Hash,
        Path,
    };

    bool ok { true };
    State state { State::Init };

    for (auto it = topic.begin(); it != topic.end(); ++it) {
        if ((state == State::Hash) && ((*it) != '/')) {
            ok = false;
            break;
        }

        switch (*it) {
        case '\0':
        case '+':
            ok = false;
            break;

        case '/':
            state = State::Separator;
            break;

            ok = false;
            break;

        case '#':
            if ((state != State::Separator)
             && (state != State::Init))
            {
                ok = false;
                break;
            }

            state = State::Hash;
            break;

        default:
            if ((state != State::Separator)
             && (state != State::Init)
             && (state != State::Path))
            {
                ok = false;
                break;
            }

            if (state == State::Separator) {
                state = State::Init;
                break;
            }

            state = State::Path;
            break;
        }

        if (!ok) {
            break;
        }
    }

    return ok;
}

// Topic suffix, specific to the app. Getters and setters are expected to be the last level of the topic.
// While it is technically allowed to have an empty level string, prefer not to have one.
bool is_valid_suffix(espurna::StringView suffix) {
    if (!suffix.length()) {
        return true;
    }

    enum class State {
        Init,
        Separator,
        Path,
    };

    State state { State::Init };

    for (auto it = suffix.begin(); it != suffix.end(); ++it) {
        switch (*it) {
        case '#':
        case '+':
        case '\0':
            return false;

        case '/':
            state = State::Separator;
            break;

        default:
            state = State::Path;
            break;
        }
    }

    return state != State::Separator;
}

char filter_wildcard(StringView filter) {
    char out = '\0';

    for (auto it = filter.begin(); it != filter.end(); ++it) {
        switch (*it) {
        case '\0':
            out = '\0';
            break;

        case '+':
        case '#':
            if (out != '\0') {
                out = '\0';
                break;
            }

            out = *it;
            break;
        }
    }

    return out;
}

bool is_valid_single_level(StringView value) {
    for (auto it = value.begin(); it != value.end(); ++it) {
        switch (*it) {
        case '\0':
        case '+':
        case '#':
        case '/':
            return false;
        }
    }

    return true;
}

bool is_valid_multi_level(StringView value) {
    for (auto it = value.begin(); it != value.end(); ++it) {
        switch (*it) {
        case '\0':
        case '+':
        case '#':
            return false;
        }
    }

    return true;
}

// Given a topic filter aka pattern, extract wildcard value from the input topic string.
// e.g.
// * <TOPIC>/#/set - generic topic placement
//           ^
// * <LHS>/#/<RHS>/set - when wildcard is manually placed
//         ^
// * #/<RHS>/set - when magnitude is at the start
//   ^
// * #/set - when wildcard is the only part of the filter
//   ^
//
// Will validate '#' and '+' result in accordance with the <ROOT> topic rules
// - '#' always matches at least one level, never the previous one
// - '+' always matches exactly one level
// (since internally we would never want MQTT magical previous level matching with '#')
//
// Returns a StringView which points to either
// - part of the 'topic' containing the match (also includes an emtpy match possibility)
// - nullptr and length of zero, when maching had failed
StringView match_wildcard(StringView filter, StringView topic, char wildcard) {
    StringView out;

    if (!topic.length() || !filter.length()) {
        return out;
    }

    switch (wildcard) {
    case '+':
    case '#':
        break;

    default:
        return out;
    }

    if (!filter.length() || ((filter.length() - 1) > topic.length())) {
        return out;
    }

    auto it = std::find(filter.begin(), filter.end(), wildcard);
    if (it == filter.end()) {
        return out;
    }

    const auto start = StringView(filter.begin(), it);

    if (start.length() && !topic.startsWith(start)) {
        return out;
    }

    if (start.length()) {
        topic = topic.slice(start.length());
    }

    const auto end = StringView(it + 1, filter.end());

    if (end.length() && !topic.endsWith(end)) {
        return out;
    }

    if (end.length()) {
        topic = topic.slice(0, topic.length() - end.length());
    }

    if ((('+' == wildcard) && is_valid_single_level(topic))
     || (('#' == wildcard) && is_valid_multi_level(topic)))
    {
        out = topic;
    }

    return out;
}

} // namespace

} // namespace mqtt
} // namespace espurna
