#pragma once

#include <cstdint>
#include <random>
#include <type_traits>

namespace distsysenv {

enum class RandomSeed : uint64_t {
  kDEFAULT = 0,
};

[[nodiscard]] constexpr RandomSeed MakeRandomSeed(uint64_t value) noexcept {
  return static_cast<RandomSeed>(value);
}

class Random {
 public:
  Random()
      : seed_(static_cast<RandomSeed_t>(RandomSeed::kDEFAULT)),
        gen_(mix_seed(seed_)) {}

  explicit Random(RandomSeed seed)
      : seed_(static_cast<RandomSeed_t>(seed)), gen_(mix_seed(seed_)) {}

  explicit Random(uint64_t raw_seed) : seed_(raw_seed), gen_(mix_seed(raw_seed)) {}

  template <typename T>
  T uniform(T a, T b) {
    if constexpr (std::is_integral_v<T>) {
      std::uniform_int_distribution<T> dist(a, b);
      return dist(gen_);
    } else if constexpr (std::is_floating_point_v<T>) {
      std::uniform_real_distribution<T> dist(a, b);
      return dist(gen_);
    } else {
      static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
      return {};
    }
  }

 private:
  using RandomSeed_t = std::underlying_type_t<RandomSeed>;

  static std::mt19937::result_type mix_seed(uint64_t s) {
    return static_cast<std::mt19937::result_type>(s ^
                                                  (s >> 32) ^ (s >> 48));
  }

  RandomSeed_t seed_;
  std::mt19937 gen_;
};

}  // namespace distsysenv
