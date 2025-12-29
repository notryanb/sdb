#include<sys/ptrace.h>
#include <libsdb/breakpoint_site.hpp>
#include<libsdb/process.hpp>
#include<libsdb/error.hpp>

namespace {
  // a function-local static variable that is initialized exactly once.
  // Because it is 'static', this exists for the lifetime of the program
  auto get_next_id() {
    static sdb::breakpoint_site::id_type id = 0;
    return ++id;
  }
}

void sdb::breakpoint_site::enable() {
  if (is_enabled_) return;

  // ptrace doesn't return an errno, but it sets it as a side effect.
  // We set it to 0 before the call and then check afterwards.
  errno = 0;
  std::uint64_t data = ptrace(PTRACE_PEEKDATA, process_->pid(), address_, nullptr);
  if (errno != 0) {
    error::send_errno("Enabling breakpoint site failed");
  }

  // only need the first byte of the 64 bits and persist it into the breakpoint
  saved_data_ = static_cast<std::byte>(data & 0xff);

  // prep the data with the breakpoint instruction, int3.
  // Remmeber, the `data` is a u64, but we only want to replace the last byte (little endian).
  // The rest of the 64 bit integer needs to stay intact. Using ~0xFF will keep the rest of the
  // number and then OR with 0xCC will set 0xCC at the last byte (or first... depending if you're already thinking in little endian)
  std::uint64_t int3 = 0xcc;
  std::uint64_t data_with_int3 = ((data & ~0xff) | int3);

  if (ptrace(PTRACE_POKEDATA, process_->pid(), address_, data_with_int3) < 0) {
    error::send_errno("Enabling breakpoint site failed");
  }

  is_enabled_ = true;
}

sdb::breakpoint_site::breakpoint_site(process& proc, virt_addr address)
  : process_{ &proc }, address_{ address }, is_enabled_{ false },saved_data_{} {
    id_ = get_next_id();
  }
