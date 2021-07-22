// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../sys/platform.h"

#include <limits>

#define _USE_MATH_DEFINES
#include <math.h> // using cmath causes issues under Windows
#include <cfloat>
#include <climits>

namespace embree
{
  static MAYBE_UNUSED const float one_over_255 = 1.0f/255.0f;
  static MAYBE_UNUSED const float min_rcp_input = 1E-18f;  // for abs(x) >= min_rcp_input the newton raphson rcp calculation does not fail

  /* we consider floating point numbers in that range as valid input numbers */
  static MAYBE_UNUSED float FLT_LARGE = 1.844E18f;

  struct TrueTy {
    __forceinline operator bool( ) const { return true; }
  };

  const constexpr TrueTy True = TrueTy();

  struct FalseTy {
    __forceinline operator bool( ) const { return false; }
  };

  const constexpr FalseTy False = FalseTy();
  
  struct ZeroTy
  {
    __forceinline operator          double   ( ) const { return 0; }
    __forceinline operator          float    ( ) const { return 0; }
    __forceinline operator          long long( ) const { return 0; }
    __forceinline operator unsigned long long( ) const { return 0; }
    __forceinline operator          long     ( ) const { return 0; }
    __forceinline operator unsigned long     ( ) const { return 0; }
    __forceinline operator          int      ( ) const { return 0; }
    __forceinline operator unsigned int      ( ) const { return 0; }
    __forceinline operator          short    ( ) const { return 0; }
    __forceinline operator unsigned short    ( ) const { return 0; }
    __forceinline operator          char     ( ) const { return 0; }
    __forceinline operator unsigned char     ( ) const { return 0; }
  }; 

  const constexpr ZeroTy zero = ZeroTy();

  struct OneTy
  {
    __forceinline operator          double   ( ) const { return 1; }
    __forceinline operator          float    ( ) const { return 1; }
    __forceinline operator          long long( ) const { return 1; }
    __forceinline operator unsigned long long( ) const { return 1; }
    __forceinline operator          long     ( ) const { return 1; }
    __forceinline operator unsigned long     ( ) const { return 1; }
    __forceinline operator          int      ( ) const { return 1; }
    __forceinline operator unsigned int      ( ) const { return 1; }
    __forceinline operator          short    ( ) const { return 1; }
    __forceinline operator unsigned short    ( ) const { return 1; }
    __forceinline operator          char     ( ) const { return 1; }
    __forceinline operator unsigned char     ( ) const { return 1; }
  };

  const constexpr OneTy one = OneTy();

  struct NegInfTy
  {
    __forceinline operator          double   ( ) const { return -std::numeric_limits<double>::infinity(); }
    __forceinline operator          float    ( ) const { return -std::numeric_limits<float>::infinity(); }
    __forceinline operator          long long( ) const { return std::numeric_limits<long long>::min(); }
    __forceinline operator unsigned long long( ) const { return std::numeric_limits<unsigned long long>::min(); }
    __forceinline operator          long     ( ) const { return std::numeric_limits<long>::min(); }
    __forceinline operator unsigned long     ( ) const { return std::numeric_limits<unsigned long>::min(); }
    __forceinline operator          int      ( ) const { return std::numeric_limits<int>::min(); }
    __forceinline operator unsigned int      ( ) const { return std::numeric_limits<unsigned int>::min(); }
    __forceinline operator          short    ( ) const { return std::numeric_limits<short>::min(); }
    __forceinline operator unsigned short    ( ) const { return std::numeric_limits<unsigned short>::min(); }
    __forceinline operator          char     ( ) const { return std::numeric_limits<char>::min(); }
    __forceinline operator unsigned char     ( ) const { return std::numeric_limits<unsigned char>::min(); }

  };

  const constexpr NegInfTy neg_inf = NegInfTy();

  struct PosInfTy
  {
    __forceinline operator          double   ( ) const { return std::numeric_limits<double>::infinity(); }
    __forceinline operator          float    ( ) const { return std::numeric_limits<float>::infinity(); }
    __forceinline operator          long long( ) const { return std::numeric_limits<long long>::max(); }
    __forceinline operator unsigned long long( ) const { return std::numeric_limits<unsigned long long>::max(); }
    __forceinline operator          long     ( ) const { return std::numeric_limits<long>::max(); }
    __forceinline operator unsigned long     ( ) const { return std::numeric_limits<unsigned long>::max(); }
    __forceinline operator          int      ( ) const { return std::numeric_limits<int>::max(); }
    __forceinline operator unsigned int      ( ) const { return std::numeric_limits<unsigned int>::max(); }
    __forceinline operator          short    ( ) const { return std::numeric_limits<short>::max(); }
    __forceinline operator unsigned short    ( ) const { return std::numeric_limits<unsigned short>::max(); }
    __forceinline operator          char     ( ) const { return std::numeric_limits<char>::max(); }
    __forceinline operator unsigned char     ( ) const { return std::numeric_limits<unsigned char>::max(); }
  };

  const constexpr PosInfTy     inf = PosInfTy();
  const constexpr PosInfTy pos_inf = PosInfTy();

  struct NaNTy
  {
    __forceinline operator double( ) const { return std::numeric_limits<double>::quiet_NaN(); }
    __forceinline operator float ( ) const { return std::numeric_limits<float>::quiet_NaN(); }
  };

  const constexpr NaNTy nan = NaNTy();

  struct UlpTy
  {
    __forceinline operator double( ) const { return std::numeric_limits<double>::epsilon(); }
    __forceinline operator float ( ) const { return std::numeric_limits<float>::epsilon(); }
  };
  
  const constexpr UlpTy ulp = UlpTy();

  struct PiTy
  {
    __forceinline operator double( ) const { return double(M_PI); }
    __forceinline operator float ( ) const { return float(M_PI); }
  };

  const constexpr PiTy pi = PiTy();

  struct OneOverPiTy
  {
    __forceinline operator double( ) const { return double(M_1_PI); }
    __forceinline operator float ( ) const { return float(M_1_PI); }
  };

  const constexpr OneOverPiTy one_over_pi = OneOverPiTy();

  struct TwoPiTy
  {
    __forceinline operator double( ) const { return double(2.0*M_PI); }
    __forceinline operator float ( ) const { return float(2.0*M_PI); }
  };

  const constexpr TwoPiTy two_pi = TwoPiTy();

  struct OneOverTwoPiTy
  {
    __forceinline operator double( ) const { return double(0.5*M_1_PI); }
    __forceinline operator float ( ) const { return float(0.5*M_1_PI); }
  };

  const constexpr OneOverTwoPiTy one_over_two_pi = OneOverTwoPiTy();

  struct FourPiTy
  {
    __forceinline operator double( ) const { return double(4.0*M_PI); } 
    __forceinline operator float ( ) const { return float(4.0*M_PI); }
  };

  const constexpr FourPiTy four_pi = FourPiTy();

  struct OneOverFourPiTy
  {
    __forceinline operator double( ) const { return double(0.25*M_1_PI); }
    __forceinline operator float ( ) const { return float(0.25*M_1_PI); }
  };

  const constexpr OneOverFourPiTy one_over_four_pi = OneOverFourPiTy();

  struct StepTy {
    __forceinline operator          double   ( ) const { return 0; }
    __forceinline operator          float    ( ) const { return 0; }
    __forceinline operator          long long( ) const { return 0; }
    __forceinline operator unsigned long long( ) const { return 0; }
    __forceinline operator          long     ( ) const { return 0; }
    __forceinline operator unsigned long     ( ) const { return 0; }
    __forceinline operator          int      ( ) const { return 0; }
    __forceinline operator unsigned int      ( ) const { return 0; }
    __forceinline operator          short    ( ) const { return 0; }
    __forceinline operator unsigned short    ( ) const { return 0; }
    __forceinline operator          char     ( ) const { return 0; }
    __forceinline operator unsigned char     ( ) const { return 0; }
  };

  const constexpr StepTy step = StepTy();

  struct ReverseStepTy {
  };

  const constexpr ReverseStepTy reverse_step = ReverseStepTy();

  struct EmptyTy {
  };

  const constexpr EmptyTy empty = EmptyTy();

  struct FullTy {
  };

  const constexpr FullTy full = FullTy();

  struct UndefinedTy {
  };

  const constexpr UndefinedTy undefined = UndefinedTy();

#if defined(__aarch64__)
  extern const uint32x4_t movemask_mask;
  extern const uint32x4_t vzero;
  extern const uint32x4_t v0x80000000;
  extern const uint32x4_t v0x7fffffff;
  extern const uint32x4_t v000F;
  extern const uint32x4_t v00F0;
  extern const uint32x4_t v00FF;
  extern const uint32x4_t v0F00;
  extern const uint32x4_t v0F0F;
  extern const uint32x4_t v0FF0;
  extern const uint32x4_t v0FFF;
  extern const uint32x4_t vF000;
  extern const uint32x4_t vF00F;
  extern const uint32x4_t vF0F0;
  extern const uint32x4_t vF0FF;
  extern const uint32x4_t vFF00;
  extern const uint32x4_t vFF0F;
  extern const uint32x4_t vFFF0;
  extern const uint32x4_t vFFFF;
  extern const uint8x16_t v0022;
  extern const uint8x16_t v1133;
  extern const uint8x16_t v0101;
  extern const float32x4_t vOne;
  extern const float32x4_t vmOne;
  extern const float32x4_t vInf;
  extern const float32x4_t vmInf;
#endif
}
