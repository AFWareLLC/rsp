#include "afware/rsp/API.hpp"

#include <iostream>

//
// This is a simple example to show basic usage.
// If RSP_ENABLE is defined, the profiler will be compiled in.
// If ProfilingAvailable() returns false (i.e. your machine does not
// meet the requirements, or we can't determine if that's true),
// nothing will happen.
//
// In the first scope: we demonstrate a simple scope with metadata.
// In the second scope: we introduce nested scopes.
//
// Pay attention to the print ordering.
//

int main() {
  if (rsp::ProfilingAvailable()) {
    std::cout << "Profiling enabled.\n";
    rsp::RSPInstance().SetSinkToCout();
  } else {
    std::cout << "Profiling not available\n";
  }

  if (rsp::StartProfiling()) {
    std::cout << "Profiling started.\n";
  }

  {
    RSP_SCOPE("Scope 1");
    RSP_SCOPE_METADATA("Some value 1", std::uint8_t{255});
    RSP_SCOPE_METADATA("Some other value", std::uint8_t{1});
  }

  {
    RSP_SCOPE("Parent Scope");

    {
      RSP_SCOPE("Child scope");
      RSP_SCOPE_METADATA("Some other value", std::uint8_t{1});
    }

    RSP_SCOPE_METADATA("Some value 1", std::uint8_t{255});
  }

  return 0;
}
