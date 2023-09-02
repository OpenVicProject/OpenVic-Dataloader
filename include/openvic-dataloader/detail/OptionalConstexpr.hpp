#pragma once

// THANK YOU APPLE FOR YOUR UTTER DISREGARD FOR C++20

#if __cpp_lib_optional >= 202106L
#define OVDL_OPTIONAL_CONSTEXPR constexpr
#else
#define OVDL_OPTIONAL_CONSTEXPR inline
#endif