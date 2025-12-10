#include "afware/rsp/API.hpp"

#include <cmath>
#include <chrono>
#include <iostream>
#include <iomanip>

namespace pi {

long double gauss_legendre_pi(int iterations) {
  long double a = 1.0L;
  long double b = 1.0L / std::sqrt(2.0L);
  long double t = 0.25L;
  long double p = 1.0L;

  for (int i = 0; i < iterations; i++) {
    long double a_next = (a + b) * 0.5L;
    long double b_next = std::sqrt(a * b);
    long double diff   = a - a_next;
    t -= p * diff * diff;
    p *= 2.0L;
    a = a_next;
    b = b_next;
  }

  return (a + b) * (a + b) / (4.0L * t);
}

long double borwein_quartic_pi(int iterations) {
  long double y = std::sqrt(2.0L) - 1.0L;
  long double a = 6.0L - 4.0L * std::sqrt(2.0L);

  for (int i = 0; i < iterations; i++) {
    long double y4     = y * y * y * y;
    long double root   = std::pow(1.0L - y4, 0.25L);
    long double y_next = (1.0L - root) / (1.0L + root);
    long double a_next = a * (1.0L + y_next) * (1.0L + y_next) * (1.0L + y_next) * (1.0L + y_next) -
                         std::pow(2.0L, 2.0L * i + 3.0L) * y_next * (1.0L + y_next + y_next * y_next);
    y = y_next;
    a = a_next;
  }

  return 1.0L / a;
}

}  // namespace pi

int main() {
  if (rsp::Available()) {
    std::cout << "Profiling enabled.\n";
  } else {
    std::cout << "Profiling unavailable.\n";
  }

  if (rsp::Start()) {
    std::cout << "Profiling started.\n";
  }

  const int runs = 100000;
  auto start     = std::chrono::high_resolution_clock::now();

  {
    RSP_SCOPE("Compute Gauss-Legendre Pi 100x");
    long double pi_gl = 0.0L;
    for (int i = 0; i < runs; i++) {
      RSP_SCOPE("Gauss-Legendre single run");
      RSP_SCOPE_METADATA("GL run", i);
      pi_gl = pi::gauss_legendre_pi(5);
    }

    (void)pi_gl;
  }
  {
    RSP_SCOPE("Compute Borwein Quartic Pi 100x");
    long double pi_borwein = 0.0L;
    for (int i = 0; i < runs; i++) {
      RSP_SCOPE("Borwein quartic single run");
      RSP_SCOPE_METADATA("BQ run", i);
      pi_borwein = pi::borwein_quartic_pi(4);
    }

    (void)pi_borwein;
  }

  auto end                              = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;

  std::cout << "Time doing actual work: " << elapsed.count() << " seconds\n";

  rsp::Stop();

  return 0;
}
