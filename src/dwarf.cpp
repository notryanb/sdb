#include <libsdb/bit.hpp>
#include <libsdb/dwarf.hpp>
#include <libsdb/elf.hpp>
#include <libsdb/types.hpp>

#include <algorithm>
#include <string_view>


namespace {
  class cursor {
    public:
      explicit cursor(sdb::span<const std::byte> data) : data_(data), pos_(data.begin()) {}

      cursor& operator++() { ++pos_; return *this; }
      cursor& operator+=(std::size_t size) { pos_ += size; return *this; }

      const std::byte* position() const { return pos_; }

      bool finished() const { return pos_ >= data_.end(); }

      template <class T>
      T fixed_int() {
        auto t = sdb::from_bytes<T>(pos_);
        pos_ += sizeof(T);
        return t;
      }

      std::uint8_t u8() { return fixed_int<std::uint8_t>(); }
      std::uint16_t u16() { return fixed_int<std::uint16_t>(); }
      std::uint32_t u32() { return fixed_int<std::uint32_t>(); }
      std::uint64_t u64() { return fixed_int<std::uint64_t>(); }
      std::int8_t s8() { return fixed_int<std::int8_t>(); }
      std::int16_t s16() { return fixed_int<std::int16_t>(); }
      std::int32_t s32() { return fixed_int<std::int32_t>(); }
      std::int64_t s64() { return fixed_int<std::int64_t>(); }

      std::string_view string() {
        auto null_terminator = std::find(pos_, data_.end(), std::byte{0});
        std::string_view ret(reinterpret_cast<const char*>(pos_), null_terminator - pos_);
        pos_ = null_terminator + 1;
        return ret;
      }

      std::uint64_t uleb128() {
        std::uint64_t res = 0;
        int shift = 0;
        std::uint8_t byte = 0;

        do {
          byte = u8();
          auto masked = static_cast<std::uint64_t>(byte & 0x7f);
          res |= masked << shift;
          shift += 7;
        } while ((byte & 0x80) != 0);

        return res;
      }

      std::int64_t sleb128() {
        std::uint64_t res = 0;
        int shift = 0;
        std::uint8_t byte = 0;

        do {
          byte = u8();
          auto masked = static_cast<std::uint64_t>(byte & 0x7f);
          res |= masked << shift;
          shift += 7;
        } while ((byte & 0x80) != 0);

        if ((shift < sizeof(res) * 8) and (byte & 0x40)) {
          res |= (~static_cast<std::uint64_t>(0) << shift);
        }

        return res;
      }

    private:
      sdb::span<const std::byte> data_;
      const std::byte* pos_;
  };

  std::unordered_map<std::uint64_t, sdb::abbrev> parse_abbrev_table(const sdb::elf& obj, std::size_t offset) {
    cursor cur(obj.get_section_contents(".debug_abbrev"));
    cur += offset;

    std::unordered_map<std::uint64_t, sdb::abbrev> table;
    std::uint64_t code = 0;
    do {
      // parse one entry
    } while (code != 0);

    return table;
  }

}

const std::unordered_map<std::uint64_t, sdb::abbrev>& sdb::dwarf::get_abbrev_table(std::size_t offset) {
  if (!abbrev_tables_.count(offset)) {
    abbrev_tables_.emplace(offset, parse_abbrev_table(*elf_, offset));
  }

  return abbrev_tables_.at(offset);
}
