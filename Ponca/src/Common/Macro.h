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

#define PONCA_TODO checkf(false, TEXT("TODO"))
