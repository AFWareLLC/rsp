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
// Lastly, we demonstrate RSP_FUNCTION_SCOPE, which creates a scope for
// the entire function and names it accordingly.
//

void MyFunction() {
  RSP_FUNCTION_SCOPE;
  RSP_SCOPE_METADATA("Some function value", std::uint8_t{1});
}

int main() {
  if (rsp::Available()) {
    std::cout << "Profiling enabled.\n";
    rsp::Instance().SetSinkToCout();
  } else {
    std::cout << "Profiling not available\n";
  }

  if (rsp::Start()) {
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

  MyFunction();

  rsp::Stop();

  return 0;
}
