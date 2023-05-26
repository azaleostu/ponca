#pragma once

#include "CoreMinimal.h"

#define PONCA_ERROR                checkf(false, "")
#define PONCA_ERROR_MSG(MSG)       checkf(false, (MSG))
#define PONCA_ASSERT(EXPR)         check(EXPR)
#define PONCA_ASSERT_MSG(EXPR,MSG) checkf((EXPR), (MSG))

#define PONCA_DEBUG_ERROR                 checkfSlow(false, "")
#define PONCA_DEBUG_ERROR_MSG(MSG)        checkfSlow(false, (MSG))
#define PONCA_DEBUG_ASSERT(EXPR)          checkSlow(EXPR)
#define PONCA_DEBUG_ASSERT_MSG(EXPR,MSG)  checkfSlow((EXPR), (MSG))
