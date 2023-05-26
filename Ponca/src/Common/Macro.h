#pragma once

#include <cstdio>

// macro delimiters
#define PONCA_MACRO_START do {
#define PONCA_MACRO_END   } while(0)

// Stringification
#define PONCA_XSTR(S) #S
#define PONCA_STR(S) PONCA_XSTR(S)

// legacy
#define PONCA_CRASH
#define PONCA_PRINT_ERROR(MSG)
#define PONCA_PRINT_WARNING(MSG)

// turnoff warning
#define PONCA_UNUSED(VAR)                                                       \
    PONCA_MACRO_START                                                           \
    (void)(VAR);                                                               \
    PONCA_MACRO_END

#define PONCA_TODO checkf(false, "TODO")

#ifdef __has_builtin
#if __has_builtin(__builtin_clz)
#define PONCA_HAS_BUILTIN_CLZ 1
#endif
#endif

#ifndef PONCA_HAS_BUILTIN_CLZ
#define PONCA_HAS_BUILTIN_CLZ 0
#endif
