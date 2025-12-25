#include <cstdio>
#include <vector>
#include <chrono>
#include "universal_rng.h"

static double now_ms() {
  using namespace std::chrono;
  return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

static void bench_algo(ua::Algorithm algo, const char* name) {
  ua::Init init;
  init.algo = algo;
  init.seed = 0xDEADBEEFCAFEBABEULL;
  init.stream = 7;
  init.buffer.capacity_u64 = 4096;

  ua::UniversalRng rng(init);

  constexpr std::size_t N = 1ull << 24; // 16M u64
  std::vector<std::uint64_t> u(N);
  std::vector<double> d(N / 2), nrm(N / 2);

  double t0 = now_ms(); rng.generate_u64(u.data(), u.size());
  double t1 = now_ms(); rng.generate_double(d.data(), d.size());
  double t2 = now_ms(); rng.generate_normal(0.0, 1.0, nrm.data(), nrm.size());
  double t3 = now_ms();

  std::uint64_t acc = 0; for (auto x : u) acc ^= x;
  double accd = 0.0; for (auto x : d) accd += x;
  double mean = 0.0; for (auto x : nrm) mean += x; mean /= nrm.size();

  std::printf("[%s] u64: %7.2f ms (%.2f M/s)  dbl: %7.2f ms (%.2f M/s)  nrm: %7.2f ms (%.2f M/s)\n",
              name,
              (t1 - t0), (u.size() / 1e6) / ((t1 - t0) / 1000.0),
              (t2 - t1), (d.size() / 1e6) / ((t2 - t1) / 1000.0),
              (t3 - t2), (nrm.size() / 1e6) / ((t3 - t2) / 1000.0));
  std::printf(" checksum=%016llx  sumdbl=%.3f  mean(nrm)=%.6f\n",
              (unsigned long long)acc, accd, mean);

  // quick skip-ahead smoke test: skip 1e6 blocks and ensure different numbers
  std::uint64_t before = rng.next_u64();
  rng.skip_ahead_blocks(1000000ULL);
  std::uint64_t after = rng.next_u64();
  std::printf(" skip_ahead sanity: before=%016llx after=%016llx\n",
              (unsigned long long)before, (unsigned long long)after);
}

int main() {
  bench_algo(ua::Algorithm::Xoshiro256ss, "xoshiro256**");
  bench_algo(ua::Algorithm::Philox4x32_10, "philox4x32-10");
  return 0;
}
