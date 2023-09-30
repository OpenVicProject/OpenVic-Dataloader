#pragma once

// THANK YOU APPLE FOR YOUR UTTER DISREGARD FOR C++20

#if defined(__cpp_lib_optional) && __cpp_lib_optional >= 202106L
#define OVDL_OPTIONAL_CONSTEXPR constexpr
#else
#define OVDL_OPTIONAL_CONSTEXPR inline
#endif

#if defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907L
#define OVDL_STRING_CONSTEXPR constexpr
#else
#define OVDL_STRING_CONSTEXPR inline
#endif

#if defined(__cpp_lib_optional) && __cpp_lib_optional >= 202106L && defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907L
#define OVDL_STR_OPT_CONSTEXPR constexpr
#else
#define OVDL_STR_OPT_CONSTEXPR inline
#endif

#if defined(__cpp_lib_constexpr_vector) && __cpp_lib_constexpr_vector >= 201907L
#define OVDL_VECTOR_CONSTEXPR constexpr
#else
#define OVDL_VECTOR_CONSTEXPR inline
#endif