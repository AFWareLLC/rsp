#include "afware/rsp/API.hpp"

#include <filesystem>
#include <iostream>

int main() {
  if (rsp::Available()) {
    std::cout << "Profiling enabled.\n";

    auto sink_ptr = rsp::Profiler::CreateBinaryDiskSink("/tmp/rsp_example.bin");
    rsp::Instance().SetSinkToBinaryDisk(sink_ptr);
  } else {
    std::cout << "Profiling not available\n";
  }

  if (rsp::Start()) {
    std::cout << "Profiling started.\n";
  }

  for (std::size_t i = 0; i < 1000; ++i) {
    RSP_SCOPE("Loop example");
    RSP_SCOPE_METADATA("Loop counter", i);
  }

  std::cout << "Done\n";

  rsp::Stop();

  return 0;
}
