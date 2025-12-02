#include "afware/rsp/API.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>

using namespace rsp;

int main() {
  std::ifstream fd("/tmp/rsp_example.bin", std::ios::binary);
  if (!fd) {
    std::cerr << "Could not open file. Did you run disk_producer first?\n";
    return 1;
  }

  size_t count = 0;

  while (true) {
    uint32_t len = 0;
    fd.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (fd.eof()) break;
    if (!fd) {
      std::cerr << "Failed to read length prefix\n";
      break;
    }

    std::vector<uint8_t> buffer(len);
    fd.read(reinterpret_cast<char*>(buffer.data()), len);
    if (fd.gcount() != len) {
      std::cerr << "Failed to read full FlatBuffer of length " << len << "\n";
      break;
    }

    flatbuffers::Verifier verifier(buffer.data(), len);
    if (!RSP::VerifyScopeInfoBuffer(verifier)) {
      std::cerr << "FlatBuffer verification failed for record #" << count << "\n";
      continue;
    }

    const RSP::ScopeInfo* scope = RSP::GetScopeInfo(buffer.data());
    if (!scope) {
      std::cerr << "Failed to parse FlatBuffer for record #" << count << "\n";
      continue;
    }

    std::cout << "---------------------------------\n";
    std::cout << "#" << count++ << "\n";
    std::cout << scope << "\n";
  }

  std::cout << "Read " << count << " records\n";
  return 0;
}
