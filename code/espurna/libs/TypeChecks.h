// -----------------------------------------------------------------------------
// Detection idiom adapted from "Working Draft, C++ Extensions for Library Fundamentals, Version 3":
// https://cplusplus.github.io/fundamentals-ts/v3.html#meta.detect
// -----------------------------------------------------------------------------

#pragma once

#include <type_traits>

// In case we do support c++17 just use the headers shipped with the GCC
#if __cplusplus >= 201703L
#include <type_traits>
#include <experimental/type_traits>
using std::experimental::is_detected;
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

    template <template<class...> class Op, class... Args>
    using detected_t = typename implementation::detector<implementation::nonesuch, void, Op, Args...>::type;

    template <class Default, template<class...> class Op, class... Args>
    using detected_or = implementation::detector<Default, void, Op, Args...>;

    // ...
    // implement the rest as needed. some things may not work though
}

using experimental_type_traits::is_detected;

#endif // __cplusplus >= 201703L
