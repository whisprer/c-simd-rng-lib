#include "universal_rng.h"
#include "runtime_detect.h"
#include "rng_distributions.h"
#include "simd_normal.h"
#include <atomic>
#include <mutex>
#include <cstring>

namespace ua {
struct Xoshiro256ssScalar;
#if defined(UA_ENABLE_AVX2)
struct Xoshiro256ssAVX2;
struct Philox4x32_10_AVX2;
#endif
#if defined(UA_ENABLE_AVX512)
struct Xoshiro256ssAVX512;
struct Philox4x32_10_AVX512;
#endif
struct Philox4x32_10_Scalar;
#if defined(__aarch64__) || defined(__ARM_NEON)
struct Philox4x32_10_NEON;
#endif
}

namespace ua {

class GeneratorContext {
public:
  explicit GeneratorContext(const Init& init)
  : algo_(init.algo), simd_(detect_simd()), buffer_cap_(init.buffer.capacity_u64) {
    if (buffer_cap_ == 0) buffer_cap_ = 1024;
    buffer_.resize(buffer_cap_);
    seed_ = init.seed;
    stream_ = init.stream;
    ZigguratNormal::ensure_init();
    construct_backend();
    refill();
  }
  ~GeneratorContext() { destroy_backend(); }

  void skip_ahead_blocks(std::uint64_t nblocks) {
    skip_backend(nblocks);
    idx_ = size_ = 0; // invalidate buffer (conservative correctness)
  }

  std::uint64_t next_u64() { if (idx_ >= size_) refill(); return buffer_[idx_++]; }
  double        next_double(){ return u64_to_unit_double(next_u64()); }

  void set_capacity(std::size_t cap) {
    if (cap == 0) cap = 1024;
    buffer_cap_ = cap;
    buffer_.assign(buffer_cap_, 0);
    idx_ = size_ = 0;
    refill();
  }

  void generate_u64(std::uint64_t* dst, std::size_t n) {
    while (n) {
      if (idx_ < size_) {
        std::size_t take = size_ - idx_;
        if (take > n) take = n;
        std::memcpy(dst, buffer_.data() + idx_, take * sizeof(std::uint64_t));
        idx_ += take; dst += take; n -= take;
      } else {
        if (n >= buffer_cap_) { backend_fill(dst, buffer_cap_); dst += buffer_cap_; n -= buffer_cap_; }
        else refill();
      }
    }
  }

  void generate_double(double* dst, std::size_t n) {
    const std::size_t tmpN = 1024;
    std::uint64_t tmp[tmpN];
    while (n) {
      std::size_t take = (n > tmpN) ? tmpN : n;
      generate_u64(tmp, take);
      for (std::size_t i = 0; i < take; ++i) dst[i] = u64_to_unit_double(tmp[i]);
      dst += take; n -= take;
    }
  }

  void generate_normal(double mean, double stddev, double* dst, std::size_t n) {
    if (n == 0) return;
    const std::size_t B = 8192;

#if defined(UA_ENABLE_AVX512)
    if (simd_ == SimdTier::AVX512) {
      std::vector<std::uint64_t> u(B), v(B);
      std::size_t left = n;
      while (left) {
        std::size_t take = left > B ? B : left;
        generate_u64(u.data(), take);
        generate_u64(v.data(), take);
        simd_normal_avx512(u.data(), v.data(), take, mean, stddev, dst);
        dst += take; left -= take;
      }
      return;
    }
#endif
#if defined(UA_ENABLE_AVX2)
    if (simd_ == SimdTier::AVX2) {
      std::vector<std::uint64_t> u(B), v(B);
      std::size_t left = n;
      while (left) {
        std::size_t take = left > B ? B : left;
        generate_u64(u.data(), take);
        generate_u64(v.data(), take);
        simd_normal_avx2(u.data(), v.data(), take, mean, stddev, dst);
        dst += take; left -= take;
      }
      return;
    }
#endif
#if defined(__aarch64__) || defined(__ARM_NEON)
    {
      std::vector<std::uint64_t> u(B), v(B);
      std::size_t left = n;
      while (left) {
        std::size_t take = left > B ? B : left;
        generate_u64(u.data(), take);
        generate_u64(v.data(), take);
        simd_normal_neon(u.data(), v.data(), take, mean, stddev, dst);
        dst += take; left -= take;
      }
      return;
    }
#endif
    // scalar fallback
    const std::size_t tmpN = 1024;
    std::uint64_t u[tmpN], v[tmpN];
    while (n) {
      std::size_t take = (n > tmpN) ? tmpN : n;
      generate_u64(u, take);
      generate_u64(v, take);
      for (std::size_t i = 0; i < take; ++i) {
        double u01 = u64_to_unit_double(u[i]);
        double extra = u64_to_unit_double(v[i]);
        double z = ZigguratNormal::sample(u[i], u01, extra);
        dst[i] = mean + stddev * z;
      }
      dst += take; n -= take;
    }
  }

private:
  Algorithm   algo_;
  SimdTier    simd_;
  std::uint64_t seed_{}, stream_{};
  std::vector<std::uint64_t> buffer_;
  std::size_t buffer_cap_{1024};
  std::size_t idx_{0}, size_{0};

  alignas(64) unsigned char backend_mem_[256];
  enum class BackendTag : unsigned char {
    None, Scalar, AVX2, AVX512, PhiloxScalar, PhiloxAVX2, PhiloxAVX512, PhiloxNEON
  } tag_{BackendTag::None};

  void construct_backend();
  void destroy_backend() noexcept;
  void backend_fill(std::uint64_t* dst, std::size_t n);
  void skip_backend(std::uint64_t nblocks);

  void refill() { backend_fill(buffer_.data(), buffer_cap_); idx_ = 0; size_ = buffer_cap_; }
};

void GeneratorContext::construct_backend() {
  switch (algo_) {
    case Algorithm::Xoshiro256ss: {
      if (simd_ == SimdTier::AVX512) {
#if defined(UA_ENABLE_AVX512)
        new (backend_mem_) Xoshiro256ssAVX512(seed_, stream_);
        tag_ = BackendTag::AVX512; break;
#endif
      }
      if (simd_ == SimdTier::AVX2) {
#if defined(UA_ENABLE_AVX2)
        new (backend_mem_) Xoshiro256ssAVX2(seed_, stream_);
        tag_ = BackendTag::AVX2; break;
#endif
      }
      new (backend_mem_) Xoshiro256ssScalar(seed_, stream_);
      tag_ = BackendTag::Scalar;
    } break;

    case Algorithm::Philox4x32_10: {
#if defined(UA_ENABLE_AVX512)
      if (simd_ == SimdTier::AVX512) { new (backend_mem_) Philox4x32_10_AVX512(seed_, stream_); tag_ = BackendTag::PhiloxAVX512; break; }
#endif
#if defined(UA_ENABLE_AVX2)
      if (simd_ == SimdTier::AVX2)   { new (backend_mem_) Philox4x32_10_AVX2(seed_, stream_);   tag_ = BackendTag::PhiloxAVX2; break; }
#endif
#if defined(__aarch64__) || defined(__ARM_NEON)
      { new (backend_mem_) Philox4x32_10_NEON(seed_, stream_); tag_ = BackendTag::PhiloxNEON; break; }
#endif
      new (backend_mem_) Philox4x32_10_Scalar(seed_, stream_);
      tag_ = BackendTag::PhiloxScalar;
    } break;
  }
}

void GeneratorContext::destroy_backend() noexcept {
  switch (tag_) {
    case BackendTag::Scalar:
      reinterpret_cast<Xoshiro256ssScalar*>(backend_mem_)->~Xoshiro256ssScalar(); break;
#if defined(UA_ENABLE_AVX2)
    case BackendTag::AVX2:
      reinterpret_cast<Xoshiro256ssAVX2*>(backend_mem_)->~Xoshiro256ssAVX2(); break;
    case BackendTag::PhiloxAVX2:
      reinterpret_cast<Philox4x32_10_AVX2*>(backend_mem_)->~Philox4x32_10_AVX2(); break;
#endif
#if defined(UA_ENABLE_AVX512)
    case BackendTag::AVX512:
      reinterpret_cast<Xoshiro256ssAVX512*>(backend_mem_)->~Xoshiro256ssAVX512(); break;
    case BackendTag::PhiloxAVX512:
      reinterpret_cast<Philox4x32_10_AVX512*>(backend_mem_)->~Philox4x32_10_AVX512(); break;
#endif
#if defined(__aarch64__) || defined(__ARM_NEON)
    case BackendTag::PhiloxNEON:
      reinterpret_cast<Philox4x32_10_NEON*>(backend_mem_)->~Philox4x32_10_NEON(); break;
#endif
    case BackendTag::PhiloxScalar:
      reinterpret_cast<Philox4x32_10_Scalar*>(backend_mem_)->~Philox4x32_10_Scalar(); break;
    default: break;
  }
  tag_ = BackendTag::None;
}

void GeneratorContext::backend_fill(std::uint64_t* dst, std::size_t n) {
  switch (tag_) {
    case BackendTag::Scalar:
      reinterpret_cast<Xoshiro256ssScalar*>(backend_mem_)->fill_u64(dst, n); break;
#if defined(UA_ENABLE_AVX2)
    case BackendTag::AVX2:
      reinterpret_cast<Xoshiro256ssAVX2*>(backend_mem_)->fill_u64(dst, n); break;
    case BackendTag::PhiloxAVX2:
      reinterpret_cast<Philox4x32_10_AVX2*>(backend_mem_)->fill_u64(dst, n); break;
#endif
#if defined(UA_ENABLE_AVX512)
    case BackendTag::AVX512:
      reinterpret_cast<Xoshiro256ssAVX512*>(backend_mem_)->fill_u64(dst, n); break;
    case BackendTag::PhiloxAVX512:
      reinterpret_cast<Philox4x32_10_AVX512*>(backend_mem_)->fill_u64(dst, n); break;
#endif
#if defined(__aarch64__) || defined(__ARM_NEON)
    case BackendTag::PhiloxNEON:
      reinterpret_cast<Philox4x32_10_NEON*>(backend_mem_)->fill_u64(dst, n); break;
#endif
    case BackendTag::PhiloxScalar:
      reinterpret_cast<Philox4x32_10_Scalar*>(backend_mem_)->fill_u64(dst, n); break;
    default:
      reinterpret_cast<Xoshiro256ssScalar*>(backend_mem_)->fill_u64(dst, n); break;
  }
}

void GeneratorContext::skip_backend(std::uint64_t nblocks) {
  switch (tag_) {
    case BackendTag::PhiloxScalar:
      reinterpret_cast<Philox4x32_10_Scalar*>(backend_mem_)->skip_ahead_blocks(nblocks); break;
#if defined(UA_ENABLE_AVX2)
    case BackendTag::PhiloxAVX2:
      reinterpret_cast<Philox4x32_10_AVX2*>(backend_mem_)->skip_ahead_blocks(nblocks); break;
#endif
#if defined(UA_ENABLE_AVX512)
    case BackendTag::PhiloxAVX512:
      reinterpret_cast<Philox4x32_10_AVX512*>(backend_mem_)->skip_ahead_blocks(nblocks); break;
#endif
#if defined(__aarch64__) || defined(__ARM_NEON)
    case BackendTag::PhiloxNEON:
      reinterpret_cast<Philox4x32_10_NEON*>(backend_mem_)->skip_ahead_blocks(nblocks); break;
#endif
    default:
      // xoshiro fallback: generate/discard 2*nblocks u64s to advance roughly same amount
      if (nblocks) {
        std::uint64_t sink;
        const std::uint64_t toss = 2ULL * nblocks;
        for (std::uint64_t i=0;i<toss;++i) (void)next_u64();
      }
  }
}

// thread-local facade

static thread_local GeneratorContext* tls_ctx = nullptr;
static std::once_flag init_once_guard;

static void init_default_once() {
  if (!tls_ctx) { Init def{}; tls_ctx = new GeneratorContext(def); }
}

void rng_init(const Init& init) {
  if (tls_ctx) { delete tls_ctx; tls_ctx = nullptr; }
  tls_ctx = new GeneratorContext(init);
}

void rng_set_buffer_capacity(std::size_t capacity_u64) {
  std::call_once(init_once_guard, &init_default_once);
  tls_ctx->set_capacity(capacity_u64);
}

void rng_skip_ahead_blocks(std::uint64_t nblocks) {
  std::call_once(init_once_guard, &init_default_once);
  tls_ctx->skip_ahead_blocks(nblocks);
}

std::uint64_t rng_next_u64() { std::call_once(init_once_guard, &init_default_once); return tls_ctx->next_u64(); }
double        rng_next_double(){ std::call_once(init_once_guard, &init_default_once); return tls_ctx->next_double(); }
void          rng_generate_u64(std::uint64_t* d,std::size_t n){ std::call_once(init_once_guard,&init_default_once); tls_ctx->generate_u64(d,n); }
void          rng_generate_double(double* d,std::size_t n){ std::call_once(init_once_guard,&init_default_once); tls_ctx->generate_double(d,n); }
void          rng_generate_normal(double mean, double stddev, double* d, std::size_t n) {
  std::call_once(init_once_guard, &init_default_once);
  tls_ctx->generate_normal(mean, stddev, d, n);
}

// RAII

struct UniversalRng::Impl { GeneratorContext ctx; explicit Impl(const Init& i):ctx(i){} };
UniversalRng::UniversalRng(const Init& i):p_(std::make_unique<Impl>(i)){}
UniversalRng::~UniversalRng() = default;

void UniversalRng::skip_ahead_blocks(std::uint64_t n){ p_->ctx.skip_ahead_blocks(n); }
std::uint64_t UniversalRng::next_u64(){ return p_->ctx.next_u64(); }
double        UniversalRng::next_double(){ return p_->ctx.next_double(); }
void          UniversalRng::generate_u64(std::uint64_t* d,std::size_t n){ p_->ctx.generate_u64(d,n); }
void          UniversalRng::generate_double(double* d,std::size_t n){ p_->ctx.generate_double(d,n); }
void          UniversalRng::generate_normal(double mean, double stddev, double* d, std::size_t n){ p_->ctx.generate_normal(mean,stddev,d,n); }

} // namespace ua
