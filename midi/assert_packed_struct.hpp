#pragma once
#include <cassert>
#include <concepts>
#define ASSERT_PACKED_STRUCT(Type, ExpectedSize) \
    static_assert(sizeof(Type) == ExpectedSize); \
    static_assert(is_standard_layout_v<Type>); \
    static_assert(is_trivially_copyable_v<Type>)
