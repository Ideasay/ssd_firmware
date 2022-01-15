
#ifndef __DEBUG_H_
#define __DEBUG_H_

#include "assert.h"

#define __ASSERT 1
#define ASSERT_BRANCH_TMP

#if __ASSERT
#define ASSERT(X)														\
if (!(X))																\
{																		\
	xil_printf("\r\n\nerror in %s: Line %d\r\n", __FILE__, __LINE__);	\
	while(1) ;															\
}
#else
#define ASSERT(X)
#endif

#ifdef ASSERT_BRANCH_TMP
#define ASSERT_TMP  ASSERT()
#else
#define ASSERT_TMP(...)
#endif
#endif

