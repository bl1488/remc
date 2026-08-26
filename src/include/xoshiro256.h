#ifndef XOSHIRO256_H_
#define XOSHIRO256_H_

#include <random>

class Xoshiro256Generator {
public:
   Xoshiro256Generator() noexcept {
      std::random_device rd;
      for (auto& i : array_)
         i = static_cast<uint64_t>(rd()) << 32 | rd();
   }

   constexpr uint64_t operator()() noexcept {
      const uint64_t result = Rotl(array_[1] * 5, 7) * 9;
      const uint64_t tmp    = array_[1] << 17;
      
      array_[2] ^= array_[0]; array_[3] ^= array_[1];
      array_[1] ^= array_[2]; array_[0] ^= array_[3];
      array_[2] ^= tmp;       array_[3] = Rotl(array_[3], 45);

      return result;
   }

private:
   constexpr uint64_t Rotl(const uint64_t x, int k)
      const noexcept { return (x << k) | (x >> (64 - k)); }

private:
   uint64_t array_[4];
};

#endif // XOSHIRO256_RANDOM_H_
