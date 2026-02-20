#include <int_arrays.h>

// Define macros for UI16
#define T_TYPE uint16_t
#define T_STRUCT UI16_ARRAY
#define T_FUNC(name) ui16_##name
#define T_TEST_FUNC TEST_UI16_ARRAY
#define T_NAME "UI16_ARRAY"
#define T_VAL(i) ((uint16_t)((i) * 100))
#define T_RESIZE_VAL 9999
#define T_FMT "%d"
#define T_CAST(val) (val)
#include "templates/test_int_array_template.inc"
#undef T_TYPE
#undef T_STRUCT
#undef T_FUNC
#undef T_TEST_FUNC
#undef T_NAME
#undef T_VAL
#undef T_RESIZE_VAL
#undef T_FMT
#undef T_CAST

// Define macros for UI32
#define T_TYPE uint32_t
#define T_STRUCT UI32_ARRAY
#define T_FUNC(name) ui32_##name
#define T_TEST_FUNC TEST_UI32_ARRAY
#define T_NAME "UI32_ARRAY"
#define T_VAL(i) ((uint32_t)((i) * 1000))
#define T_RESIZE_VAL 999999
#define T_FMT "%d"
#define T_CAST(val) (val)
#include "templates/test_int_array_template.inc"
#undef T_TYPE
#undef T_STRUCT
#undef T_FUNC
#undef T_TEST_FUNC
#undef T_NAME
#undef T_VAL
#undef T_RESIZE_VAL
#undef T_FMT
#undef T_CAST

// Define macros for UI64
#define T_TYPE uint64_t
#define T_STRUCT UI64_ARRAY
#define T_FUNC(name) ui64_##name
#define T_TEST_FUNC TEST_UI64_ARRAY
#define T_NAME "UI64_ARRAY"
#define T_VAL(i) ((uint64_t)((i) * 1000000))
#define T_RESIZE_VAL 999999999999ULL
#define T_FMT "%llu"
#define T_CAST(val) ((unsigned long long)(val))
#include "templates/test_int_array_template.inc"
#undef T_TYPE
#undef T_STRUCT
#undef T_FUNC
#undef T_TEST_FUNC
#undef T_NAME
#undef T_VAL
#undef T_RESIZE_VAL
#undef T_FMT
#undef T_CAST
