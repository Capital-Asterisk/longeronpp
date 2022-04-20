/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once


#if defined(LGRN_ASSERT_DISABLE) || defined(NDEBUG)
    #define LGRN_ASSERT(expr) ((void)0)
    #define LGRN_ASSERTV(expr, ...) ((void)0)
    #define LGRN_ASSERTM(expr, msg) ((void)0)
    #define LGRN_ASSERTMV(expr, msg, ...) ((void)0)
#elif defined(LGRN_ASSERT_CUSTOM)
    LGRN_ASSERT_CUSTOM()
#elif defined(LGRN_ASSERT_C)
    #include <cassert>
    #define LGRN_ASSERT(expr) assert((expr));
    #define LGRN_ASSERTM(expr, msg) assert(((void)msg, expr));
    #define LGRN_ASSERTV(expr, ...) assert(expr);
    #define LGRN_ASSERTMV(expr, msg, ...) assert(((void)msg, expr));
#else
    #include <cstdlib>
    #include <iostream>

    #ifdef __GNUC__
        #define _LGRN_FUNC() static_cast<char const*>(__PRETTY_FUNCTION__)
    #elif defined(_MSC_VER)
        #define _LGRN_FUNC() __FUNCSIG__
    #endif

    // Customizable extra information
    #ifndef LGRN_ASSERT_DECORATE
        #define LGRN_ASSERT_DECORATE() ""
    #endif

    // Macro nonsense
    #define _LGRN_SLIDE_10(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, OUT, ...) OUT

    // Allow printing 6 extra vars (or entire expressions) on assert
    #define _LGRN_VAR_1(a) "  * " << (#a) << ": " << (a) << std::endl
    #define _LGRN_VAR_2(a, b) _LGRN_VAR_1(a) << _LGRN_VAR_1(b)
    #define _LGRN_VAR_3(a, b, c) _LGRN_VAR_1(a) << _LGRN_VAR_2(b, c)
    #define _LGRN_VAR_4(a, b, c, d) _LGRN_VAR_1(a) << _LGRN_VAR_3(b, c, d)
    #define _LGRN_VAR_5(a, b, c, d, e) _LGRN_VAR_1(a) << _LGRN_VAR_4(b, c, d, e)
    #define _LGRN_VAR_6(a, b, c, d, e, f) _LGRN_VAR_1(a) << _LGRN_VAR_5(b, c, d, e, f)

    // Call a _LGRN_VAR_N depending on number of __VA_ARGS__
    #define _LGRN_VAR(...) "* Vars:" << std::endl << _LGRN_SLIDE_10( \
            __VA_ARGS__, 0, 0, 0, 0, _LGRN_VAR_6, _LGRN_VAR_5, _LGRN_VAR_4, \
            _LGRN_VAR_3, _LGRN_VAR_2, _LGRN_VAR_1)(__VA_ARGS__)

    #define _LGRN_HEADER(msg) std::endl << std::endl << msg << std::endl
    #define _LGRN_INFO()   "* File: " << __FILE__ << std::endl \
                        << "* Line: " << __LINE__ << std::endl \
                        << "* Func: " << _LGRN_FUNC() << std::endl

    // Assert 'template'
    #define _LGRN_ASSERT(expr, msg, extra)                  \
    if (!(expr)) [[unlikely]]                               \
    {                                                       \
        std::cerr << _LGRN_HEADER(msg) << _LGRN_INFO()      \
                  << "* Expr: " << (#expr) << std::endl     \
                  << extra << LGRN_ASSERT_DECORATE();       \
        std::abort();                                       \
    }                                                       \
    ((void)0) // require semicolon

    // Usable assert functions
    #define LGRN_ASSERT(expr) _LGRN_ASSERT(expr, "Assertion Failed!", "")
    #define LGRN_ASSERTV(expr, ...) _LGRN_ASSERT(expr, "Assertion Failed!", _LGRN_VAR(__VA_ARGS__))
    #define LGRN_ASSERTM(expr, msg) _LGRN_ASSERT(expr, "Assertion Failed: " msg, "")
    #define LGRN_ASSERTMV(expr, msg, ...) _LGRN_ASSERT(expr, "Assertion Failed: " msg, _LGRN_VAR(__VA_ARGS__))
#endif
