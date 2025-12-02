#include "afware/rsp/API.hpp"

#include <iostream>
#include <thread>
#include <vector>

//
// This spins up a bunch of threads and tortures the
// profiler. We should see nice looking output here - no crashes,
// no corruption.
//

void worker(int num) {
  std::cout << "Started thread " << num << "\n";
  for (size_t i = 0; i < 10000; ++i) {
    RSP_SCOPE("Worker Loop");
    RSP_SCOPE_METADATA("Thread", num);
    RSP_SCOPE_METADATA("Count", i);
  }

  std::cout << "Finished thread " << num << "\n";
}

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

  std::vector<std::thread> threads;
  for (int i = 0; i < 48; ++i) {
    threads.push_back(std::thread{worker, i});
  }

  for (auto &t : threads) {
    t.join();
  }

  return 0;
}
