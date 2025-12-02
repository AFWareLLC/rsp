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

#define RSP_CONCAT_IMPL(a, b) a##b
#define RSP_CONCAT(a, b) RSP_CONCAT_IMPL(a, b)

#define RSP_SCOPE_IMPL(TAG_STR) ::rsp::ActiveScope RSP_CONCAT(_active_scope_, __COUNTER__)(TAG_STR)

#define RSP_SCOPE_METADATA_IMPL(TAG_STR, VALUE)                      \
  do {                                                               \
    auto* current = ::rsp::GetScopeManager()->Current();             \
    if (current) {                                                   \
      current->info.AddMetadata(::rsp::MetadataTag(TAG_STR), VALUE); \
    }                                                                \
  } while (0)
