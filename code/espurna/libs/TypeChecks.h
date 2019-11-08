// This code is part of the GNU ISO C++ Library distributed with GCC-9.2.0
// Copyright (C) 2007-2019 Free Software Foundation, Inc.
//
// type_traits & experimental/type_traits modified to be distributed with ESPurna

#pragma once

#include <type_traits>

// In case we do support c++17 just use the headers shipped with the GCC
#if __cplusplus >= 201703L
#include <experimental/type_traits>
using std::is_detected;
#else

namespace experimental_type_traits {
    namespace implementation {
        // Small workaround for GCC-4.8.2 to *really* trigger substitution error with void_t
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64395
        // https://stackoverflow.com/a/35754473
        template <typename... >
        struct make_void {
            using type = void;
        };

        template <typename... T>
        using void_t = typename make_void<T...>::type;

        // Return dummy type as the Default type
        struct nonesuch {
            nonesuch() = delete;
            ~nonesuch() = delete;
            nonesuch(nonesuch const&) = delete;
            void operator=(nonesuch const&) = delete;
        };

        // Implementation of the detection idiom (negative case).
        template<typename Default, typename AlwaysVoid,
            template<typename...> class Op, typename... Args>
        struct detector {
            using value_t = std::false_type;
            using type = Default;
        };

        // Implementation of the detection idiom (positive case).
        template<typename Default, template<typename...> class Op,
            typename... Args>
        struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
            using value_t = std::true_type;
            using type = Op<Args...>;
        };
    }

    template<template<typename...> class Op, typename... Args>
    using is_detected = typename implementation::detector<implementation::nonesuch, void, Op, Args...>::value_t;

    // ...
    // implement the rest as needed. some things may not work though
}

using experimental_type_traits::is_detected;

#endif // __cplusplus >= 201703L
