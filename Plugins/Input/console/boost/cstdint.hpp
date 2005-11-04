
// Boost substitute. For full boost library see http://boost.org

#ifndef BOOST_CSTDINT_HPP
#define BOOST_CSTDINT_HPP

#if BLARGG_USE_NAMESPACE
#	include <climits>
#else
#	include <limits.h>
#endif

BLARGG_BEGIN_NAMESPACE (boost)

#include "sys/types.h"

BLARGG_END_NAMESPACE

#endif

