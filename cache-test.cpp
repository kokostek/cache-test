/* Random memory access measurement, showing CPU cache size impact.
 *
 * Compile and run:
 *   > c++ cache-test.cpp -o cache-test -std=c++20 -O1 && ./cache-test
 *
 * Example output on i9-9900K (32+32KB L1, 256KB L2, 16MB Smart Cache), with
 * comments:
 * size_in_bytes     ticks_per_item
 * --------------------------------
 *  1024                      3.7907
 *  2048                     3.86114
 *  4096                     3.84755
 *  8192                     3.76617
 *  16384                     3.7394
 *  32768                    3.72742  // 32KB L1
 *  65536                    6.51552
 *  131072                   7.83066
 *  262144                   13.2395  // 256KB L2
 *  524288                   25.9217
 *  1048576                  33.7032
 *  2097152                  32.1329
 *  4194304                  33.8009
 *  8388608                  34.3796
 *  16777216                 57.5418  // 16MB Smart
 *  33554432                 154.609
 *  67108864                 184.766
 *  134217728                219.106
 *  268435456                230.216
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <random>
#include <span>
#include <vector>

/* Using __rdtsc intrinsic here. */
#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

size_t hop_read(const std::span<const size_t> items, const size_t num_hops) {

  size_t next = 0;

  for (size_t i = 0; i < num_hops; i++) {
    /* No bounds check: assuming that items are properly set. */
    next = items[next];
  }

  /* Need to return something, so compiler does not optimize away the loop. */
  return next;
}

int main(const int argc, const char *argv[]) {

  /* memory size in bytes */
  static constexpr size_t sizes[] = {
      // KBs
      1 << 10,
      1 << 11,
      1 << 12,
      1 << 13,
      1 << 14,
      1 << 15,
      1 << 16,
      1 << 17,
      1 << 18,
      1 << 19,

      // MBs
      1 << 20,
      1 << 21,
      1 << 22,
      1 << 23,
      1 << 24,
      1 << 25,
      1 << 26,
      1 << 27,
      1 << 28,
  };

  static constexpr auto colw = 16;

  /* clang-format off */
  std::cout
    << std::left << std::setw(colw) << "size_in_bytes"
    << std::right << std::setw(colw) << "ticks_per_item"
    << std::endl;
  /* clang-format on */

  /* clang-format off */
  std::cout
    << std::setfill('-') << std::setw(colw) << ""
    << std::setfill('-') << std::setw(colw) << ""
    << std::endl;
  /* clang-format on */

  std::cout << std::setfill(' ');

  size_t result = 0;

  for (const auto sz : sizes) {

    /* accounting for element size */
    const size_t elems = sz / sizeof(size_t);

    std::vector<size_t> items(elems);

    const auto seed =
        std::chrono::system_clock::now().time_since_epoch().count();

    std::default_random_engine gen(seed);

    for (size_t i = 0; i < elems; i++) {
      items[i] = i;
    }

    std::shuffle(items.begin(), items.end(), gen);

    size_t num_hops = static_cast<size_t>(100) * 1000 * 1000;

    if (elems > num_hops)
      num_hops = elems;

    const auto start_tick = __rdtsc();
    result += hop_read(items, num_hops);
    const auto total_ticks = __rdtsc() - start_tick;
    const auto ticks_per_item = (double)total_ticks / num_hops;

    // clang-format off
    std::cout
      << std::left << std::setw(colw) << sz
      << std::right << std::setw(colw) << ticks_per_item
      << std::endl;
    // clang-format on
  }

  /* priting this, so compiler does not optimize away the loop: */
  std::cout << "(result: " << result << ")" << std::endl;

  return 0;
}
