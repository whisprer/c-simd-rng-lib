// bench/bench_lock_and_bench.cpp
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <cstdint>
#include <cstring>
#include "universal_rng.h"

using clock_t = std::chrono::steady_clock;

static inline double ms_now() {
  using namespace std::chrono;
  return duration<double, std::milli>(clock_t::now().time_since_epoch()).count();
}

// SplitMix64 fold to make stable 64-bit checksums quickly
static inline std::uint64_t splitmix64(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

static std::uint64_t hash_u64_span(const std::uint64_t* p, std::size_t n) {
  std::uint64_t h = 0x123456789ABCDEF0ULL;
  for (std::size_t i = 0; i < n; ++i) {
    h = splitmix64(h ^ p[i]);
  }
  return h;
}

static std::uint64_t hash_double_span(const double* p, std::size_t n) {
  std::uint64_t h = 0xCAFEBABEDEADBEEFULL;
  for (std::size_t i = 0; i < n; ++i) {
    std::uint64_t u;
    std::memcpy(&u, &p[i], sizeof(u));
    h = splitmix64(h ^ u);
  }
  return h;
}

static void bench_one(ua::Algorithm algo, const char* name,
                      std::uint64_t seed, std::uint64_t stream,
                      std::size_t N_u64, std::size_t N_double, std::size_t N_norm) {
  ua::Init init;
  init.algo = algo;
  init.seed = seed;
  init.stream = stream;
  init.buffer.capacity_u64 = 8192;

  ua::UniversalRng rng(init);

  std::vector<std::uint64_t> u(N_u64);
  std::vector<double> d(N_double);
  std::vector<double> nrm(N_norm);

  // u64
  double t0 = ms_now();
  rng.generate_u64(u.data(), u.size());
  double t1 = ms_now();

  // double
  rng.generate_double(d.data(), d.size());
  double t2 = ms_now();

  // normal()
  rng.generate_normal(0.0, 1.0, nrm.data(), nrm.size());
  double t3 = ms_now();

  // checksums
  std::uint64_t hu = hash_u64_span(u.data(), u.size());
  std::uint64_t hd = hash_double_span(d.data(), d.size());
  std::uint64_t hn = hash_double_span(nrm.data(), nrm.size());

  // simple sanity: mean & stdev for normals (rough)
  long double sum = 0.0L, sum2 = 0.0L;
  for (double x : nrm) { sum += x; sum2 += x * x; }
  double mean = static_cast<double>(sum / nrm.size());
  double var  = static_cast<double>(sum2 / nrm.size() - mean * mean);

  auto rate = [](std::size_t n, double ms) -> double {
    return ms > 0.0 ? (n / 1e6) / (ms / 1000.0) : 0.0;
  };

  std::printf("=== %s ===\n", name);
  std::printf("u64:   %8.2f ms  (%.2f M u64/s)\n", (t1 - t0), rate(u.size(), (t1 - t0)));
  std::printf("dbl:   %8.2f ms  (%.2f M dbl/s)\n", (t2 - t1), rate(d.size(), (t2 - t1)));
  std::printf("norm:  %8.2f ms  (%.2f M nrm/s)\n", (t3 - t2), rate(nrm.size(), (t3 - t2)));
  std::printf("chk: u64=%016llx  dbl=%016llx  nrm=%016llx\n",
              (unsigned long long)hu, (unsigned long long)hd, (unsigned long long)hn);
  std::printf("norm sanity: mean=%.6f  var=%.6f (should be ~0, ~1)\n\n", mean, var);
}

int main(int argc, char** argv) {
  // Config via env (so you can sweep sizes without recompiling)
  const std::size_t N_u64   = std::getenv("UA_N_U64")   ? std::strtoull(std::getenv("UA_N_U64"),   nullptr, 10) : (1ull<<24);  // 16M
  const std::size_t N_double= std::getenv("UA_N_DBL")   ? std::strtoull(std::getenv("UA_N_DBL"),   nullptr, 10) : (1ull<<23);  // 8M
  const std::size_t N_norm  = std::getenv("UA_N_NRM")   ? std::strtoull(std::getenv("UA_N_NRM"),   nullptr, 10) : (1ull<<23);  // 8M
  const std::uint64_t seed  = std::getenv("UA_SEED")    ? std::strtoull(std::getenv("UA_SEED"),    nullptr, 16) : 0xDEADBEEFCAFEBABEULL;
  const std::uint64_t stream= std::getenv("UA_STREAM")  ? std::strtoull(std::getenv("UA_STREAM"),  nullptr, 10) : 7ULL;

  std::printf("sizes: u64=%zu  dbl=%zu  nrm=%zu   seed=0x%016llx  stream=%llu\n\n",
              N_u64, N_double, N_norm,
              (unsigned long long)seed, (unsigned long long)stream);

  bench_one(ua::Algorithm::Xoshiro256ss, "xoshiro256**", seed, stream, N_u64, N_double, N_norm);
  bench_one(ua::Algorithm::Philox4x32_10, "philox4x32-10", seed, stream, N_u64, N_double, N_norm);
  return 0;
}
