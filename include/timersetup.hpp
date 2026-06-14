#pragma once
#include <algorithm>
#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64)
#include <emmintrin.h>
#include <x86intrin.h>


static inline uint64_t startClock(){
  _mm_lfence();
  return __rdtsc();
}


static inline uint64_t stopClock(){

  unsigned aux;
  auto end = __rdtscp(&aux);
  _mm_lfence();
  return end;
}

#elif defined(__aarch64__)

// No rdtsc on ARM; cntvct_el0 is the constant-rate generic timer and isb
// serializes the pipeline the way lfence does on x86.
static inline uint64_t startClock(){
  uint64_t ticks;
  __asm__ __volatile__("isb\n\tmrs %0, cntvct_el0" : "=r"(ticks) :: "memory");
  return ticks;
}


static inline uint64_t stopClock(){
  uint64_t ticks;
  __asm__ __volatile__("isb\n\tmrs %0, cntvct_el0" : "=r"(ticks) :: "memory");
  return ticks;
}

#else
#error "timersetup.hpp: unsupported architecture"
#endif


// Counter ticks per nanosecond. cntfrq_el0 reports the rate directly on ARM;
// the TSC rate is not architecturally exposed on x86 so calibrate it against
// the wall clock once.
static inline double ticksPerNs(){
  static const double rate = []{
#if defined(__aarch64__)
    uint64_t freqHz;
    __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(freqHz));
    return static_cast<double>(freqHz) / 1e9;
#else
    const auto t0 = std::chrono::steady_clock::now();
    const uint64_t c0 = startClock();
    while (std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(50)) {}
    const uint64_t c1 = stopClock();
    const auto t1 = std::chrono::steady_clock::now();
    const double elapsedNs =
        std::chrono::duration<double, std::nano>(t1 - t0).count();
    return static_cast<double>(c1 - c0) / elapsedNs;
#endif
  }();
  return rate;
}


// Min of many back-to-back start/stop pairs; a single pair can straddle a
// context switch and wildly overstate the overhead.
static inline uint64_t measureTimerOverhead(){
  uint64_t best = UINT64_MAX;
  for(int i = 0; i < 10'000; ++i){
    const uint64_t start = startClock();
    const uint64_t stop  = stopClock();
    best = std::min(best, stop - start);
  }
  return best;
}

static inline const uint64_t timerOverhead = measureTimerOverhead();


static inline uint64_t calculateCycles(uint64_t start, uint64_t stop){
  const uint64_t raw = stop - start;
  return raw > timerOverhead ? raw - timerOverhead : 0;
}

static inline double cyclesToNs(uint64_t cycles){
  return static_cast<double>(cycles) / ticksPerNs();
}

static inline double calculateNs(uint64_t start, uint64_t stop){
  return cyclesToNs(calculateCycles(start, stop));
}
