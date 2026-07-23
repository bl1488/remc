#ifndef SERIALIZER_H_
#define SERIALIZER_H_

#include <iostream>
#include <cstring>
#include <concepts>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// Some info:
//  - Doesnt translate into big-endian
//  - Serializer and Deserializer returns meta-object Node<Type>
//    check impl SDBufferBase::Node...

// TODO: 
//   - Add aligned version mb?
//   - Slice integrals optimization
//     examples:
//       std::size_t(255) -> 00 00 00 00 00 00 00 FF
//       sliced           -> FF

// ==== SDBufferBase ====
// Serializer/Deserializer interface base
class SDBufferBase {
public:
   // Common meta-object returned by functions.
   //
   // Be careful when using the NextPos() function to set the pos_ value for template types larger than 1 byte. 
   // Be sure to divide the value by sizeof(Type).
   template<typename Type>
   struct Node {      
      using value_type                  = Type;
      static constexpr std::size_t NPOS = ~std::size_t{0};
      
      Type        value{};
      std::size_t pos = NPOS;

      // PrefixSizeType is required for functions that work with buffers
      // since the first bytes store the buffer size
      template<typename PrefixSizeType = uint8_t>
      std::size_t NextPos() const noexcept { 
         if (pos == NPOS) 
              return pos;
         // align pos
         if constexpr (requires { value.size(); typename Type::value_type; })
              return AlignmentTo4((value.size() * sizeof(typename Type::value_type)) + pos + sizeof(PrefixSizeType));
         else return pos + sizeof(Type);
      }

      explicit operator bool() const noexcept { return pos != std::string::npos; }
   };

protected:
   // Reserve
   explicit SDBufferBase(std::size_t cap_size = 0) { buffer_.reserve(cap_size); }

   // Copy external buffer
   template<typename Type>
   explicit SDBufferBase(std::span<Type> data) {
      assert(data.data());
      data.empty();
      // Narrowing conversion
      auto bytes = std::as_bytes(data);
      buffer_.assign(bytes.begin(), bytes.end());
   }

   // Move external buffer 
   explicit SDBufferBase(std::vector<uint8_t>&& in_vec) : buffer_(std::move(in_vec))  {}

   SDBufferBase(const SDBufferBase& other) : buffer_(other.buffer_), pos_(other.pos_) {}

   SDBufferBase(SDBufferBase&& other) noexcept :
      buffer_(std::move(other.buffer_)), pos_(std::exchange(other.pos_, 0)) {}

   SDBufferBase& operator=(SDBufferBase other) noexcept {
      buffer_ = std::move(other.buffer_);
      pos_    = other.pos_;
      return *this;
   }

   ~SDBufferBase() = default;

public:
   // Set new position in buffer.
   // Just return pointer to new pos or nullptr (no exceptions/error codes etc)
   uint8_t* SetPos(std::size_t new_pos) noexcept {
      if (new_pos > buffer_.size())
         return nullptr;
      return buffer_.data() + (pos_ = new_pos);
   }

   std::size_t GetPos()                    const noexcept { return pos_;    }

   const std::vector<uint8_t>& GetBuffer() const noexcept { return buffer_; }

   std::vector<uint8_t>& GetBuffer()       noexcept       { return buffer_; }

protected:
   static std::size_t AlignmentTo4(std::size_t value) 
      noexcept { return (value + 3) & ~3; }

protected:
   std::vector<uint8_t> buffer_;
   std::size_t          pos_ = 0;
};

// ==== BinarySerializer ====
//
class BinarySerializer : public SDBufferBase {
public:
   explicit BinarySerializer(std::size_t cap_size = 0)      : SDBufferBase(cap_size) {}

   template<typename Type>
   explicit BinarySerializer(std::span<Type> data)          : SDBufferBase(data) {}

   explicit BinarySerializer(std::vector<uint8_t>&& in_vec) : SDBufferBase(std::move(in_vec)) {}

public:
   // For integral/floating_point
   template<typename Type> requires std::is_arithmetic_v<Type>
   Node<Type> Add(Type value, std::size_t pos) {
      // Assert on long double call
      static_assert(sizeof(Type) <= sizeof(double), 
         "Use AddRaw/AddString for long double.\n\
          Dont forget that a long double is 10 bytes, not 12 or 16"
      );
      if (!CheckBound(pos, 0, sizeof(Type)))
         return {};

      std::memcpy(buffer_.data() + pos, &value, sizeof(Type));

      return { .value = value, .pos = pos };
   }

   // Using internal pos
   template<typename Type> requires std::is_arithmetic_v<Type>
   Node<Type> Add(Type value) {
      // Assert on long double call
      static_assert(sizeof(Type) <= sizeof(double), 
         "Use AddRaw/AddString for long double.\n\
          Dont forget that a long double is 10 bytes, not 12 or 16"
      );
      if (!CheckBound(pos_, 0, sizeof(Type)))
         GrowBuffer(sizeof(Type), 0);

      std::memcpy(buffer_.data() + pos_, &value, sizeof(Type));
      pos_ += sizeof(Type);

      return { .value = value, .pos = pos_ - sizeof(Type) };
   }

   // Adding a string:
   //   <buffer size> + <buffer>
   //
   // To avoid allocating sizeof(std::string::size_type) bytes for buffer size
   // you can manually choose the size using StringSizeType template (default uint8_t)
   template<typename StringSizeType = uint8_t> requires std::integral<StringSizeType>
   Node<std::string_view> AddString(std::string_view data, std::size_t pos) {
      assert(data.data());
      // aligned buffer size
      std::size_t aligned_size = SDBufferBase::AlignmentTo4(data.size() + sizeof(StringSizeType));
      if (!CheckBound(pos, 0, aligned_size))
         return {};

      // Adding size
      auto sn = Add<StringSizeType>(data.size(), pos);
      if (sn) {
         auto next_pos = sn.template NextPos<StringSizeType>();
         // Adding buffer
         std::copy(data.begin(), data.begin() + sn.value, buffer_.data() + next_pos);
         return { .value = { reinterpret_cast<char*>(buffer_.data() + next_pos), aligned_size }, 
                  .pos   = pos };
      }
      return {};
   }

   // Using internal pos
   template<typename StringSizeType = uint8_t> requires std::integral<StringSizeType>
   Node<std::string_view> AddString(std::string_view data) {
      assert(data.data());
      // aligned buffer size
      std::size_t aligned_size = SDBufferBase::AlignmentTo4(data.size() + sizeof(StringSizeType));
      if (!CheckBound(pos_, 0, aligned_size))
         GrowBuffer(0, aligned_size);

      auto n = AddString<StringSizeType>(data, pos_);
      if (n)
         pos_ = aligned_size;
      return n;
   }

   // Almost same as AddString but allows adding a buffer of any type
   template<typename Type, typename RawBufferSizeType = uint8_t> requires std::integral<RawBufferSizeType>
   Node<std::span<Type>> AddRaw(std::span<Type> data, std::size_t pos) {
      assert(data.data());
      auto n =  AddString<RawBufferSizeType>(std::string_view{
                  reinterpret_cast<char*>(data.data()), data.size() * sizeof(Type)
                }, pos);
      if (n) {
         // Сan be change to:
         //   - (Type*)
         //   - reinterpret_cast<Type*>(const_cast<char*>(...))
         return { .value = { reinterpret_cast<Type*>(const_cast<decltype(n)::value_type::value_type*>(n.value.data())), 
                             n.value.size() / sizeof(Type) }, 
                  .pos   = pos };
      }
      return {};
   }

   // Using internal pos
   template<typename Type, typename RawBufferSizeType = uint8_t> requires std::integral<RawBufferSizeType>
   Node<std::span<Type>> AddRaw(std::span<Type> data) {
      assert(data.data());
      // aligned buffer size
      std::size_t aligned_size = SDBufferBase::AlignmentTo4(data.size() * sizeof(Type) + sizeof(RawBufferSizeType));
      if (!CheckBound(pos_, 0, aligned_size))
         GrowBuffer(0, aligned_size);
      
      auto n = AddRaw<Type, RawBufferSizeType>(data, pos_);
      if (n)
         pos_ = aligned_size;
      return n;
   }

protected:
   // Usually <prefix> is used to store sizeof(RawBufferSizeType)/sizeof(StringSizeType) 
   // in order to store the size of the buffer passed as <suffix>.
   //
   // watch AddString/AddRaw

   // Overflow protected growing.
   // Invalid data may cause an exception
   bool GrowBuffer(std::size_t prefix_extra, std::size_t suffix_extra) {
      constexpr std::size_t max = std::numeric_limits<std::size_t>::max();
      // prefix_extra + suffix_extra & sz + buffer_.size()
      if (max - prefix_extra < suffix_extra|| max - prefix_extra + suffix_extra < buffer_.size())
         return false;
      // Compute new capacity
      std::size_t sz = prefix_extra + suffix_extra + buffer_.size();
      if (sz > buffer_.capacity()) {
         // new_size * 2 shouldnt overflow.
         // Otherwise new_capacity = max => reserve() throw an exception
         std::size_t new_capacity = (sz > max / 2) ? max : sz * 2;
         buffer_.reserve(new_capacity);
      }
      buffer_.resize(sz);

      return true;
   }

   // Overflow protected boundaries check.
   bool CheckBound(std::size_t pos, std::size_t prefix, std::size_t suffix) const noexcept {
      if (pos > buffer_.size())
         return false;
      return std::numeric_limits<std::size_t>::max() - prefix >= suffix && (prefix + suffix) <= buffer_.size() - pos;
   }
};

// ==== BinaryDeserializer ====
//
class BinaryDeserializer : public SDBufferBase {
public:
   template<typename Type>
   explicit BinaryDeserializer(std::span<Type> data)          : SDBufferBase(data) {}

   explicit BinaryDeserializer(std::vector<uint8_t>&& in_vec) : SDBufferBase(std::move(in_vec)) {}

public:
   // For integral/floating_point
   template<typename Type> requires std::is_arithmetic_v<Type>
   Node<Type> Read(std::size_t pos) noexcept {
      // Check boundaries
      if (pos + sizeof(Type) > buffer_.size())
         return {};

      Node<Type> n;
      std::memcpy(&n.value, buffer_.data() + pos, sizeof(Type));
      n.pos = pos;

      return n;
   }

   // Using internal pos
   template<typename Type> requires std::integral<Type>
   Node<Type> Read() noexcept {
      auto n = Read<Type>(pos_);
      if (n)
         pos_ += sizeof(Type);
      return n;
   }

   template<typename StringSizeType = uint8_t> requires std::integral<StringSizeType>
   Node<std::string_view> ReadString(std::size_t pos) noexcept {
      // Check boundaries for buffer size
      if (pos + sizeof(StringSizeType) > buffer_.size())
         return {};
      auto buffer_size = Read<StringSizeType>(pos);               
      // Check boundaries for buffer data
      if (!buffer_size || pos + sizeof(StringSizeType) + buffer_size.value > buffer_.size()) return {};
      return {
         .value = { reinterpret_cast<char*>(buffer_.data() + pos + sizeof(StringSizeType)), buffer_size.value },
         .pos   = pos + sizeof(StringSizeType)
      };
   }

   // Using internal pos
   template<typename StringSizeType = uint8_t> requires std::integral<StringSizeType>
   Node<std::string_view> ReadString() noexcept {
      auto n = ReadString<StringSizeType>(pos_);
      if (n)
         pos_ = n.NextPos();
      return n;
   } 

   template<typename Type, typename RawBufferSizeType> requires std::integral<RawBufferSizeType>
   Node<std::span<Type>> ReadRaw(std::size_t pos) noexcept {
      auto n = ReadString<RawBufferSizeType>(pos);
      if (n) {
         // Cute cast again
         return { .value = { reinterpret_cast<Type*>(const_cast<decltype(n)::value_type::value_type*>(n.value.data())), 
                     n.value.size() / sizeof(Type) }, 
                  .pos   = pos }; 
      }
      return {};
   }

   // Using internal pos   
   template<typename Type, typename RawBufferSizeType> requires std::integral<RawBufferSizeType>
   Node<std::span<Type>> ReadRaw() noexcept {
      auto n = ReadRaw<Type, RawBufferSizeType>(pos_);
      if (n)
         pos_ = n.NextPos();
      return n;
   }
};

#endif // SERIALIZER_H_
