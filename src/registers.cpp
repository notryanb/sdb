#include <libsdb/registers.hpp>
#include <libsdb/bit.hpp>
#include <libsdb/process.hpp>
#include <iostream>

sdb::registers::value sdb::registers::read(const register_info& info) const {
  auto bytes = as_bytes(data_);

  if (info.format == register_format::uint) {
    switch (info.size) {
      case 1: return from_bytes<std::uint8_t> (bytes + info.offset);
      case 2: return from_bytes<std::uint16_t>(bytes + info.offset);
      case 4: return from_bytes<std::uint32_t>(bytes + info.offset);
      case 8: return from_bytes<std::uint64_t>(bytes + info.offset);
      default: sdb::error::send("Unexpected register size");
    }
  }
  else if (info.format == register_format::double_float) {
    return from_bytes<double>(bytes + info.offset);
  }
  else if (info.format == register_format::long_double) {
    return from_bytes<long double>(bytes + info.offset);
  }
  else if (info.format == register_format::vector and info.size == 8) {
    return from_bytes<byte64>(bytes + info.offset);
  }
  else {
    return from_bytes<byte128>(bytes + info.offset);
  }
}

void sdb::registers::write(const register_info& info, value val) {
  auto bytes = as_bytes(data_);

  std::visit([&](auto& v) {
    if (sizeof(v) == info.size) {
      auto val_bytes = as_bytes(v);
      std::copy(val_bytes, val_bytes + sizeof(v), bytes + info.offset);
    } else {
      std::cerr << "sdb::register::write call with mismatched register and value sizes";
      std::terminate();
    }
               
  }, val);

  if (info.type == register_type::fpr) {
    proc_->write_fprs(data_.i387);    
  } else {
    // 8 bit registers are offset by a byte into the super-register.
    // set the lowest 3 bits to 0, which forces 8-byte alignment
    auto aligned_offset = info.offset & ~0b111;
    proc_->write_user_area(info.offset, from_bytes<std::uint64_t>(bytes + info.offset));
  }
}
