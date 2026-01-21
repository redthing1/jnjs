#pragma once

#include <quickjs.h>

namespace jnjs {

// wrap raw JSValue to allow JSValue to be a typedef
struct raw_value {
    JSValue v;
};

} // namespace jnjs
