// Copyright © 2025, AFWare LLC <ajf@afware.io>
//
// Permission to use, copy, modify, and/or distribute this software
// for any purpose with or without fee is hereby granted, provided
// that the above copyright notice and this permission notice appear
// in all copies.
//
// THE SOFTWARE IS PROVIDED “AS IS” AND ISC DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
// DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
// ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
// OF THIS SOFTWARE.

#pragma once

#include "Macros.hpp"

#ifdef RSP_ENABLE

#include "Profiler.hpp"
#include "Serialization.hpp"
#include "Sinks.hpp"

#define RSP_SCOPE RSP_SCOPE_IMPL
#define RSP_SCOPE_METADATA RSP_SCOPE_METADATA_IMPL
#define RSP_FUNCTION_SCOPE RSP_FUNCTION_SCOPE_IMPL

namespace rsp {

inline bool Available() {
  return Instance().Ready();
}

inline bool Start() {
  return Instance().Start();
}

inline void Stop() {
  Instance().Stop();
}

}  // namespace rsp

#else

#define RSP_SCOPE(name) ((void)0)
#define RSP_SCOPE_METADATA(tag, val) ((void)0)
#define RSP_FUNCTION_SCOPE ((void)0)

namespace rsp {

inline bool Available() {
  return false;
}

inline bool Start() {
  return false;
}

inline void Stop() {
  return;
}

}  // namespace rsp

#endif
