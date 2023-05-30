#pragma once

#include "CoreMinimal.h"

#define PONCA_ERROR                checkf(false, TEXT(""))
#define PONCA_ERROR_MSG(MSG)       checkf(false, TEXT(MSG))
#define PONCA_ASSERT(EXPR)         check(EXPR)
#define PONCA_ASSERT_MSG(EXPR,MSG) checkf((EXPR), TEXT(MSG))

#define PONCA_DEBUG_ERROR                 checkfSlow(false, TEXT(""))
#define PONCA_DEBUG_ERROR_MSG(MSG)        checkfSlow(false, TEXT(MSG))
#define PONCA_DEBUG_ASSERT(EXPR)          checkSlow(EXPR)
#define PONCA_DEBUG_ASSERT_MSG(EXPR,MSG)  checkfSlow((EXPR), TEXT(MSG))
