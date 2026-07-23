#ifndef REMC_UTILS_H_
#define REMC_UTILS_H_

#include "remc_common.h"
#include "xoshiro256_random.h"

#include <chrono>
#include <cassert>
#include <cstring>
#include <functional>
#include <bit>
#include <thread>

namespace remc::utils {

template<typename TimeType = std::chrono::nanoseconds, typename ExprType, typename... ArgsType> 
   requires std::invocable<ExprType&&, ArgsType&&...>
TimeType ComputeExprTime(ExprType&& expr, ArgsType&&... args) 
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

template<typename Type>
void ZeroMemory(Type* ptr, std::size_t n) {
#if REMC_PALTFORM_WIN32
   ::SecureZeroMemory(ptr, n);
#elif REMC_PLATFORM_LINUX
   assert(ptr && n);
   std::memset(ptr, 0, n);
   // DSE
   asm volatile("" : : "r"(ptr) : "memory");
#endif
}

//
// LE
//
//
template<typename Type> requires std::integral<Type>
constexpr Type ValueToLE(Type value) noexcept {
   if constexpr (std::endian::native == std::endian::little)
        return value;
   else return std::byteswap(value);
}

template<typename Type, typename PtrType = const uint8_t*> requires std::integral<Type>
constexpr Type ReadLE(PtrType ptr) noexcept {
   Type a;
   std::memcpy(&a, ptr, sizeof(Type));
   return ValueToLE(a);
}

template<typename Type, typename PtrType = uint8_t*> requires std::integral<Type>
constexpr void WriteLE(PtrType ptr, Type value) noexcept {
   value = ValueToLE(value);
   std::memcpy(ptr, &value, sizeof(Type));
}

//
// BE
//
//
template<typename Type> requires std::integral<Type>
constexpr Type ValueToBE(Type value) noexcept {
   if constexpr (std::endian::native == std::endian::big)
        return value;
   else return std::byteswap(value);
}

template<typename Type, typename PtrType = uint8_t*> requires std::integral<Type>
constexpr Type ReadBE(PtrType ptr) noexcept {
   Type a;
   std::memcpy(&a, ptr, sizeof(Type));
   return ValueToBE(a);
}

template<typename Type, typename PtrType = uint8_t*> requires std::integral<Type>
constexpr void WriteBE(PtrType ptr, Type value) noexcept {
   value = ValueToBE(value);
   std::memcpy(ptr, &value, sizeof(Type));
}

//
// Random
//
//
template<typename Type = std::size_t> requires std::integral<Type>
Type Random() noexcept {
   static thread_local Xoshiro256Generator gen;
   return static_cast<Type>(gen());
}

template<typename Type = std::size_t> requires std::integral<Type>
Type Random(Type from, Type to) noexcept {
   assert(to > from);
   static thread_local Xoshiro256Generator gen;
   return std::uniform_int_distribution<Type>(from, to - 1)(gen);
}

} // namespace remc::utils

#endif // REMC_UTILS_H_
