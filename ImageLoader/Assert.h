#pragma once

#include <cassert>
#include <iostream>

#define ASSERT_MSG(condition, message) \
    assert(((void)(message), (condition)))

#define ASSERT(condition) \
    assert((condition))