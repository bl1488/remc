#include "include/remc_spdlog.h"
#include "net/net_common.h"
#include "net/session_base.h"
#include "crypto/chacha20.h"
#include "include/remc_utils.h"
#include "net/packet/serializer.h"

#include <cstddef>
#include <exception>
#include <memory>

#define DISABLE_TESTING__ 0

#if DISABLE_TESTING__
#  include "net/net_common.h"
#  include "net/server.h"
#else
//#  include <crypto/chacha20.h>
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
   // Init global logger
   remc::InitFileConsoleLogger(remc::DEFAULT_GLOBAL_LOGGER_NAME, 
                               remc::DEFAULT_LOGFILE_PATH);
   // Init sodium
   if (::sodium_init() < 0) {
      GlobalLogError("sodium init error");
      return 0;
   }

   struct __attribute__((packed)) A {
      char arr[4]  = { '1','2','3','4' };
      uint8_t size = 123;
   };

   BinarySerializer ser; {
      A test;
      [[maybe_unused]] auto e = ser.AddRaw<A>({ (A*)&test, 1 });
      std::cout << std::format("result:\tcur:{}\tnext:{}\n", e.pos, e.NextPos())
                << std::format("buffer:\tpos:{}\tsize:{}\n", ser.GetPos(), ser.GetBuffer().size());
      
      [[maybe_unused]] auto a = ser.AddString("hello123");
      std::cout << std::format("result:\tcur:{}\tnext:{}\n", a.pos, a.NextPos())
                << std::format("buffer:\tpos:{}\tsize:{}\n", ser.GetPos(), ser.GetBuffer().size());

      std::vector<int> vec = { 1,2,3,4,5 };
      [[maybe_unused]] auto b = ser.AddRaw<int, uint8_t>({ vec.data(), vec.size() });
      std::cout << std::format("result:\tcur:{}\tnext:{}\n", b.pos, b.NextPos())
                << std::format("buffer:\tpos:{}\tsize:{}\n", ser.GetPos(), ser.GetBuffer().size());

      [[maybe_unused]] auto c = ser.Add<int>(12345);
      std::cout << std::format("result:\tcur:{}\tnext:{}\n", c.pos, c.NextPos())
                << std::format("buffer:\tpos:{}\tsize:{}\n", ser.GetPos(), ser.GetBuffer().size());

      [[maybe_unused]] auto d = ser.Add<double>(123.321456);
      std::cout << std::format("result:\tcur:{}\tnext:{}\n", d.pos, d.NextPos())
                << std::format("buffer:\tpos:{}\tsize:{}\n", ser.GetPos(), ser.GetBuffer().size());
   }

   // for (int i = 0; i < 12; ++i) {
   //    std::cout << std::format("{}:\t<{}>[{}]\n", i, (char)ser.GetBuffer()[i], (int)ser.GetBuffer()[i]);
   // }

   // remc::net::BinaryDeserializer des(std::move(ser.GetBuffer())); {
   //    auto a = des.ReadString();
   //    std::cout << a.pos << '\t' << a.value << '\n';

   //    std::vector<int> vec;
   //    [[maybe_unused]] auto b = des.ReadRaw<int, uint8_t>();

   //    for (std::size_t i = 0; i < b.value.size(); ++i) {
   //       int value;
   //       std::memcpy(&value, (char*)b.value.data() + i * sizeof(int), sizeof(value));
   //       vec.emplace_back(value);
   //    }

   //    std::cout << des.GetPos() << '\t' << b.pos << '\t' << b.NextPos() << '\n';
   //    for (const auto& i : vec)
   //       std::cout << i << " ";
   //    std::cout << '\n';
   // }

   return 0;
}
