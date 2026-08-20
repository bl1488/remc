#ifndef REMC_UTILS_H_
#define REMC_UTILS_H_

#include "remc_common.h"
#include "xoshiro256_random.h"

#include <chrono>
#include <cassert>
#include <concepts>
#include <cstring>
#include <span>
#include <functional>
#include <bit>
#include <thread>

namespace remc::utils {

//
// Other
//
template<typename TimeType = std::chrono::nanoseconds, typename ExprType, typename... ArgsType> 
   requires std::invocable<ExprType&&, ArgsType&&...>
std::size_t ComputeExprTime(ExprType&& expr, ArgsType&&... args) 
   noexcept(std::is_nothrow_invocable_v<ExprType&&, ArgsType&&...>) 
{
   auto a = std::chrono::high_resolution_clock::now();
   std::invoke(std::forward<ExprType>(expr), std::forward<ArgsType>(args)...);
   auto b = std::chrono::high_resolution_clock::now();

   return std::chrono::duration_cast<TimeType>(b - a).count();
}

template<typename TimeType = std::chrono::milliseconds>
void Sleep(std::size_t delay) noexcept {
   std::this_thread::sleep_for(TimeType(delay));
}

[[maybe_unused]] static void SecZeroMemory(void* ptr, std::size_t n) {
#if REMC_PLATFORM_WIN32
   ::SecureZeroMemory(ptr, n);
#else
   assert(ptr);
   std::memset(ptr, 0, n);
   asm volatile("" : : "r"(ptr) : "memory");
#endif
}

template<typename Type>
std::string BufferToHexString(std::span<const Type> buffer, const char* sep = "") {
   assert(!buffer.empty());

   std::size_t size = buffer.size() * sizeof(Type);

   std::string result;
   result.reserve(size * 2);
   for (std::size_t i = 0; i < size; ++i)
      result += std::format("{:02x}{}", *(reinterpret_cast<const uint8_t*>(buffer.data()) + i), sep);

   return result;
}

//
// LE
//
template<std::integral Type>
constexpr Type ValueToLE(Type value) noexcept {
   if constexpr (std::endian::native == std::endian::little)
        return value;
   else return std::byteswap(value);
}

template<std::integral Type, typename PtrType = const uint8_t*>
constexpr Type ReadLE(PtrType ptr) noexcept {
   Type a;
   std::memcpy(&a, ptr, sizeof(Type));
   return ValueToLE(a);
}

template<std::integral Type, typename PtrType = uint8_t*>
constexpr void WriteLE(PtrType ptr, Type value) noexcept {
   value = ValueToLE(value);
   std::memcpy(ptr, &value, sizeof(Type));
}

//
// BE
//
template<std::integral Type>
constexpr Type ValueToBE(Type value) noexcept {
   if constexpr (std::endian::native == std::endian::big)
        return value;
   else return std::byteswap(value);
}

template<std::integral Type, typename PtrType = uint8_t*>
constexpr Type ReadBE(PtrType ptr) noexcept {
   Type a;
   std::memcpy(&a, ptr, sizeof(Type));
   return ValueToBE(a);
}

template<std::integral Type, typename PtrType = uint8_t*>
constexpr void WriteBE(PtrType ptr, Type value) noexcept {
   value = ValueToBE(value);
   std::memcpy(ptr, &value, sizeof(Type));
}

//
// Random
//
template<std::integral Type = std::size_t>
Type Random() noexcept {
   static thread_local Xoshiro256Generator gen;
   return static_cast<Type>(gen());
}

template<std::integral Type = std::size_t>
Type Random(Type from, Type to) noexcept {
   assert(to >= from);
   static thread_local Xoshiro256Generator gen;
   return std::uniform_int_distribution<Type>(from, to)(gen);
}

} // namespace remc::utils

#endif // REMC_UTILS_H_
