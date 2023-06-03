
#pragma once

#define UL_UTIL_ENUM_DEFINE_FLAG_OPERATORS(enum_type, base_type) \
inline constexpr enum_type operator|(const enum_type lhs, const enum_type rhs) { \
    return static_cast<const enum_type>(static_cast<const base_type>(lhs) | static_cast<const base_type>(rhs)); \
} \
inline constexpr enum_type operator&(const enum_type lhs, const enum_type rhs) { \
    return static_cast<const enum_type>(static_cast<const base_type>(lhs) & static_cast<const base_type>(rhs)); \
}
