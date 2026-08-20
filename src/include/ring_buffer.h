#ifndef REMC_RING_BUFFER_H_
#define REMC_RING_BUFFER_H_

#include <limits>
#include <memory>
#include <utility>
#include <vector>
#include <cstring>
#include <optional>
#include <span>
#include <cstdint>

namespace remc::containers {

// ===== RingBuffer =====
//
// non lock-free version.
// using in asio::post variants
class RingBuffer {
public:
   // it might throw an exception! be careful
   explicit RingBuffer(std::size_t capacity) {
      if (!capacity)
         throw std::runtime_error("capacity should be > 0");
      buffer_.resize(capacity + 1);
   }

   explicit RingBuffer(RingBuffer&& other) noexcept : 
      buffer_(std::move(other.buffer_)), 
      head_(std::exchange(other.head_, 0)),
      tail_(std::exchange(other.tail_, 0)) {}

   RingBuffer& operator=(RingBuffer&& other) noexcept {
      if (std::addressof(other) != this) {
         buffer_ = std::move(other.buffer_);
         head_   = std::exchange(other.head_, 0);
         tail_   = std::exchange(other.tail_, 0);
      }
      return *this;
   }

   RingBuffer(const RingBuffer&)            = delete;
   RingBuffer& operator=(const RingBuffer&) = delete;

   ~RingBuffer() = default;

public:
   template<std::integral PrefixSizeType = uint16_t>
   bool Push(std::span<const std::byte> data) {
      if (data.empty() || FreeSpace() < data.size() + sizeof(PrefixSizeType) || 
          data.size() > std::numeric_limits<PrefixSizeType>::max()) 
      {
         return false;
      }

      auto foo = [&](std::span<const std::byte> data) -> void {
         std::size_t space_to_end = buffer_.size() - head_;
         if (space_to_end < data.size()) {
            std::size_t delta = data.size() - space_to_end;
            // 1st part
            std::memcpy(buffer_.data() + head_, data.data(), space_to_end);
            // 2nd part
            std::memcpy(buffer_.data(), data.data() + space_to_end, delta);
            // reset head
            head_ = delta;
         }
         else {
            std::memcpy(buffer_.data() + head_, data.data(), data.size());
            // reset head
            head_ = (head_ + data.size()) % buffer_.size();
         }
      };

      // size
      auto size = static_cast<PrefixSizeType>(data.size());
      foo({ reinterpret_cast<const std::byte*>(&size), sizeof(PrefixSizeType) });
      // buffer
      foo(data);

      return true;
   }

   // returning only std::vector cuz async operations. 
   // i cannot guarantee the message lifetime when using std::span 
   // if you have such a guarantee you can add an overload for std::span
   template<std::integral PrefixSizeType = uint16_t>
   std::optional<std::vector<std::byte>> Pop() {
      if (IsEmpty())
         return std::nullopt;

      // dont use tail_ directly to preserve state when 
      // an exception occurs in std::vector
      auto foo = [&](std::span<std::byte> data, std::size_t pos) -> std::size_t {
         std::size_t space_to_end = buffer_.size() - pos;
         if (space_to_end < data.size()) {
            // 1st part
            std::memcpy(data.data(), buffer_.data() + pos, space_to_end);
            // 2nd part
            std::memcpy(data.data() + space_to_end, 
                        buffer_.data(), 
                        data.size() - space_to_end);
            return (data.size() - space_to_end) % buffer_.size();
         }
         else {
            std::memcpy(data.data(), buffer_.data() + pos, data.size());
            return (data.size() + pos) % buffer_.size();
         }
      };

      // size
      PrefixSizeType size;
      std::size_t pos = foo({ reinterpret_cast<std::byte*>(&size), sizeof(PrefixSizeType) }, tail_);
      // buffer
      std::vector<std::byte> result(size);
      pos = foo(result, pos);

      // because vector can throw an exception modify tail here
      tail_ = pos;

      return result;
   }

   void Swap(RingBuffer& other) {
      std::swap(buffer_, other.buffer_);
      std::swap(head_,   other.head_);
      std::swap(tail_,   other.tail_);
   }

   void Clear() noexcept { head_ = tail_ = 0; }

   bool IsEmpty() const noexcept { return head_ == tail_; }
   
   std::size_t GetTail() const noexcept { return tail_; }

   std::size_t GetHead() const noexcept { return head_; }

   const std::vector<std::byte>& GetBuffer() const noexcept { return buffer_; }

   std::size_t GetCapacity() 
      const noexcept { return buffer_.empty() ? 0 : buffer_.size() - 1; }

   std::size_t FreeSpace() const noexcept {
      if (buffer_.empty()) return 0;
      return (head_ >= tail_) ? 
         (buffer_.size() - head_) + tail_ - 1 : tail_ - head_ - 1;
   }

protected:
   std::vector<std::byte> 
               buffer_;
   std::size_t head_{},
               tail_{};
};

} // namespace remc::containers

#endif // REMC_RING_BUFFER_H_
