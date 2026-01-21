#pragma once

#include <quickjs.h>

#include "fwd.h"
#include "hedley.h"
#include "jnjs/raw_value.h"
#include "type_traits.h"
#include "types.h"

#if defined(JS_CHECK_JSVALUE) || (defined(JS_NAN_BOXING) && JS_NAN_BOXING)
#define JNJS_VALUE_HELPERS_CONSTEXPR constexpr
#else
#define JNJS_VALUE_HELPERS_CONSTEXPR
#endif

namespace jnjs::detail {

constexpr bool jsvalue_is_uint64 = std::is_same_v<JSValue, uint64_t>;

constexpr bool JS_IS_IN_INT32(JSValue v) {
    const auto tag = JS_VALUE_GET_TAG(v);
    return tag == JS_TAG_INT || tag == JS_TAG_BOOL || tag == JS_TAG_NULL || tag == JS_TAG_UNDEFINED;
}

template <> struct value_helpers<undefined> {
    constexpr static bool is(JSContext *, const JSValue v) { return JS_VALUE_GET_TAG(v) == JS_TAG_UNDEFINED; }
    constexpr static bool is_convertible(JSContext *c, const JSValue v) { return is(c, v); }
    constexpr static undefined as(JSContext *, JSValue) { return {}; }
    JNJS_VALUE_HELPERS_CONSTEXPR static JSValue from(JSContext *, const undefined &) { return JS_UNDEFINED; }
};

template <> struct value_helpers<null> {
    constexpr static bool is(JSContext *, const JSValue v) { return JS_VALUE_GET_TAG(v) == JS_TAG_NULL; }
    constexpr static bool is_convertible(JSContext *c, const JSValue v) { return is(c, v); }
    constexpr static null as(JSContext *, JSValue) { return {}; }
    JNJS_VALUE_HELPERS_CONSTEXPR static JSValue from(JSContext *, const null &) { return JS_NULL; }
};

template <> struct value_helpers<bool> {
    constexpr static bool is(JSContext *, const JSValue v) { return JS_VALUE_GET_TAG(v) == JS_TAG_BOOL; }
    constexpr static bool is_convertible(JSContext *, JSValue) { return true; }
    static bool as(JSContext *c, const JSValue v) {
        if (JS_IS_IN_INT32(v))
            return JS_VALUE_GET_INT(v) != 0;
        return JS_ToBool(c, v) != 0;
    }
    JNJS_VALUE_HELPERS_CONSTEXPR static JSValue from(JSContext *, const bool &v) { return JS_MKVAL(JS_TAG_BOOL, v); }
};

template <> struct value_helpers<int32_t> {
    constexpr static bool is(JSContext *, const JSValue v) { return JS_VALUE_GET_TAG(v) == JS_TAG_INT; }
    constexpr static bool is_convertible(JSContext *, JSValue) { return true; }
    static int32_t as(JSContext *c, const JSValue v) {
        if (JS_IS_IN_INT32(v))
            return JS_VALUE_GET_INT(v);
        int32_t ret;
        JS_ToInt32(c, &ret, v);
        return ret;
    }
    JNJS_VALUE_HELPERS_CONSTEXPR static JSValue from(JSContext *, const int32_t &v) { return JS_MKVAL(JS_TAG_INT, v); }
};

template <> struct value_helpers<int64_t> {
    constexpr static bool is(JSContext *, const JSValue v) {
        return JS_VALUE_GET_TAG(v) == JS_TAG_SHORT_BIG_INT || JS_VALUE_GET_TAG(v) == JS_TAG_INT;
    }
    constexpr static bool is_convertible(JSContext *, JSValue) { return true; }
    static int64_t as(JSContext *c, const JSValue v) {
        if (JS_IS_IN_INT32(v))
            return JS_VALUE_GET_INT(v);
        int64_t ret;
        JS_ToInt64(c, &ret, v);
        return ret;
    }
    static JSValue from(JSContext *c, const int64_t &v) { return JS_NewInt64(c, v); }
};

template <> struct value_helpers<uint32_t> {
    static bool is(JSContext *c, const JSValue v) { return value_helpers<int64_t>::is(c, v); }
    constexpr static bool is_convertible(JSContext *, JSValue) { return true; }
    static uint32_t as(JSContext *c, const JSValue v) {
        return static_cast<uint32_t>(value_helpers<int64_t>::as(c, v));
    }
    static JSValue from(JSContext *c, const uint32_t &v) { return JS_NewInt64(c, v); }
};

template <> struct value_helpers<uint64_t> {
    static bool is(JSContext *c, const JSValue v) { return value_helpers<int64_t>::is(c, v); }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static uint64_t as(JSContext *c, const JSValue v) {
        return static_cast<uint64_t>(value_helpers<int64_t>::as(c, v));
    }
    static JSValue from(JSContext *c, const uint64_t &v) { return JS_NewInt64(c, static_cast<int64_t>(v)); }
};

template <> struct value_helpers<double> {
    constexpr static bool is(JSContext *, const JSValue v) { return JS_VALUE_GET_TAG(v) == JS_TAG_FLOAT64; }
    constexpr static bool is_convertible(JSContext *, JSValue) { return true; }
    static double as(JSContext *c, const JSValue v) {
        if (JS_VALUE_GET_TAG(v) == JS_TAG_FLOAT64)
            return JS_VALUE_GET_FLOAT64(v);
        double ret;
        JS_ToFloat64(c, &ret, v);
        return ret;
    }
    static JSValue from(JSContext *c, const double &v) { return JS_NewFloat64(c, v); }
};

template <> struct value_helpers<raw_value> {
    static bool is(JSContext *, JSValue) { return true; }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static raw_value as(JSContext *c, const JSValue v) { return {JS_DupValue(c, v)}; }
    static JSValue from(JSContext *c, const raw_value &v) { return JS_DupValue(c, v.v); }
};

template <typename T>
struct value_helpers<T, std::enable_if_t<std::is_same_v<T, JSValue> && !jsvalue_is_uint64>> { // lol
    static bool is(JSContext *, JSValue) { return true; }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static JSValue as(JSContext *c, const JSValue v) { return JS_DupValue(c, v); }
    static JSValue from(JSContext *c, const JSValue &v) { return JS_DupValue(c, v); }
};

template <typename T> struct value_helpers<T *, std::enable_if_t<has_build_v<T>>> {
    static bool is(JSContext *, const JSValue v) { return JS_GetClassID(v) == internal_class_meta<T>::data.id; }
    static bool is_convertible(JSContext *c, const JSValue v) { return is(c, v); }
    static T *as(JSContext *, const JSValue v) {
        return static_cast<T *>(JS_GetOpaque(v, internal_class_meta<T>::data.id));
    }
    static JSValue from(JSContext *c, T *v) {
        auto ret = JS_NewObjectClass(c, internal_class_meta<T>::data.id);
        JS_SetOpaque(ret, v);
        return ret;
    }
};

template <typename T> struct value_helpers<must_be<T>> {
    static bool is(JSContext *c, JSValue v) { return value_helpers<T>::is(c, v); }
    static bool is_convertible(JSContext *c, const JSValue v) { return is(c, v); }
    static T as(JSContext *c, JSValue v) { return value_helpers<T>::as(c, v); }
    static JSValue from(JSContext *c, const T &v) { return value_helpers<T>::from(c, v); }
};

} // namespace jnjs::detail
