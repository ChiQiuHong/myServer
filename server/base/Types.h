/**
* @description: Types.h
* @author: YQ Huang
* @brief:
* @date: 2022/05/02 12:04:22
*/

#pragma once

#include <stdint.h>
#include <string.h> // memset
#include <string>

#ifndef NDEBUG
#include <assert.h>
#endif

namespace myserver {

using std::string;

inline void memZero(void* p, size_t n) {
    memset(p, 0, n);
}

template<typename To, typename From>
inline To implicit_cast(From const &f) {
    return f;
}

template<typename To, typename From>
inline To down_cast(From* f) {
    if(false) {
        implicit_cast<From*, To>(0);
    }

#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
  assert(f == NULL || dynamic_cast<To>(f) != NULL);  // RTTI: debug mode only!
#endif
  return static_cast<To>(f);

}

}   // namespace myserver