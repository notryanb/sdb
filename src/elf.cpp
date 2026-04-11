#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libsdb/elf.hpp>
#include <libsdb/error.hpp>
#include <libsdb/bit.hpp>

sdb::elf::elf(const std::filesystem::path& path) {
  path_ = path;

  if ((fd_ = open(path.c_str(), O_RDONLY)) < 0) {
    error::send_errno("Could not open ELF file");
  }

  // Need to specify 'struct' because 'stat' already exists as a function in the global namespace
  struct stat stats;
  if (fstat(fd_, &stats) < 0) {
    error::send_errno("Could not retrieve ELF file stats");
  }
  file_size_ = stats.st_size;

  void* ret;

  // 0 as the last argument lets MMAP choose where to map the memory
  if ((ret = mmap(0, file_size_, PROT_READ, MAP_SHARED, fd_, 0)) == MAP_FAILED) {
    close(fd_);
    error::send_errno("Could not mmap ELF file");
  }

  data_ = reinterpret_cast<std::byte*>(ret);

  std::copy(data_, data_ + sizeof(header_), as_bytes(header_));
}

sdb::elf::~elf() {
  munmap(data_, file_size_);
  close(fd_);
}
