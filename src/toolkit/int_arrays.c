/**
 * @file int_arrays.c
 * @brief Implementation of dynamic integer array module for 16-bit, 32-bit, and 64-bit unsigned integers.
 *
 * This file provides the complete implementation of UI16_ARRAY, UI32_ARRAY, and UI64_ARRAY functions
 * including memory management, automatic capacity growth, data integrity verification via
 * SHA-256 hashing, and binary file I/O with automatic hash validation.
 *
 * ## Implementation Notes
 * - All functions include NULL pointer checking
 * - Automatic capacity doubling (2x growth) ensures amortized O(1) append operations
 * - File I/O operations automatically compute and validate SHA-256 hashes
 * - Memory is managed with proper cleanup on errors
 * - SHA-256 hashing uses OpenSSL library
 *
 * ## Performance Characteristics
 * - init(): O(1)
 * - push() (amortized): O(1)
 * - push() (worst case): O(n) during resize
 * - Hash operations: O(n)
 * - File I/O: O(n)
 *
 * @author iZprime.com
 * @date October 2025
 * @version 1.0
 * @see int_arrays.h for API documentation
 * @ingroup iz_arrays
 */

#include <int_arrays.h>

// ========================================================================
// UI16_ARRAY IMPLEMENTATION
// ========================================================================
/// @cond IZ_ARRAY_TEMPLATE_MACROS
#define TEMPLATE_TYPE uint16_t
#define TEMPLATE_STRUCT UI16_ARRAY
#define TEMPLATE_FUNC(name) ui16_##name
#define TEMPLATE_NAME_STR "UI16_ARRAY"
/// @endcond
#include "templates/int_array_impl.inc"
/// @cond IZ_ARRAY_TEMPLATE_MACROS
#undef TEMPLATE_TYPE
#undef TEMPLATE_STRUCT
#undef TEMPLATE_FUNC
#undef TEMPLATE_NAME_STR
/// @endcond

// ========================================================================
// UI32_ARRAY IMPLEMENTATION
// ========================================================================
/// @cond IZ_ARRAY_TEMPLATE_MACROS
#define TEMPLATE_TYPE uint32_t
#define TEMPLATE_STRUCT UI32_ARRAY
#define TEMPLATE_FUNC(name) ui32_##name
#define TEMPLATE_NAME_STR "UI32_ARRAY"
/// @endcond
#include "templates/int_array_impl.inc"
/// @cond IZ_ARRAY_TEMPLATE_MACROS
#undef TEMPLATE_TYPE
#undef TEMPLATE_STRUCT
#undef TEMPLATE_FUNC
#undef TEMPLATE_NAME_STR
/// @endcond

// ========================================================================
// UI64_ARRAY IMPLEMENTATION
// ========================================================================
/// @cond IZ_ARRAY_TEMPLATE_MACROS
#define TEMPLATE_TYPE uint64_t
#define TEMPLATE_STRUCT UI64_ARRAY
#define TEMPLATE_FUNC(name) ui64_##name
#define TEMPLATE_NAME_STR "UI64_ARRAY"
/// @endcond
#include "templates/int_array_impl.inc"
/// @cond IZ_ARRAY_TEMPLATE_MACROS
#undef TEMPLATE_TYPE
#undef TEMPLATE_STRUCT
#undef TEMPLATE_FUNC
#undef TEMPLATE_NAME_STR
/// @endcond
