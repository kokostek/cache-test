/* Random memory access measurement, showing CPU cache size impact.
 *
 * Compile and run:
 *   > c++ cache-test.cpp -o cache-test -std=c++20 -O1 && ./cache-test
 *
 * Example output on i9-9900K (32+32KB L1, 256KB L2, 16MB Smart Cache), with
 * comments:
 *   size_in_bytes     ticks_per_item          result
 *   ------------------------------------------------
 *   1024                     3.93812              50
 *   2048                     3.90921             100
 *   4096                     3.91112              97
 *   8192                     3.91105             928
 *   16384                    3.91337              51
 *   32768                    3.96422            3803  // 32KB L1
 *   65536                    7.02569            3288
 *   131072                   10.8894            2137
 *   262144                   14.6336           21127  // 256KB L2
 *   524288                   25.6436           54523
 *   1048576                  33.0112          124363
 *   2097152                  30.2302           81499
 *   4194304                  31.6043            8934
 *   8388608                  37.1856          626178
 *   16777216                  119.13          670803  // 16MB Smart
 *   33554432                 194.757         3736844
 *   67108864                 217.386         7939310
 *   134217728                234.904        16688526
 *   268435456                234.535         1792626
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
    << std::right << std::setw(colw) << "result"
    << std::endl;
  /* clang-format on */

  /* clang-format off */
  std::cout
    << std::setfill('-') << std::setw(colw) << ""
    << std::setfill('-') << std::setw(colw) << ""
    << std::setfill('-') << std::setw(colw) << ""
    << std::endl;
  /* clang-format on */

  std::cout << std::setfill(' ');

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
    const auto result = hop_read(items, num_hops);
    const auto total_ticks = __rdtsc() - start_tick;
    const auto ticks_per_item = (double)total_ticks / num_hops;

    // clang-format off
    std::cout
      << std::left << std::setw(colw) << sz
      << std::right << std::setw(colw) << ticks_per_item
      /* priting this, so compiler does not optimize away the loop: */
      << std::right << std::setw(colw) << result
      << std::endl;
    // clang-format on
  }

  return 0;
}
