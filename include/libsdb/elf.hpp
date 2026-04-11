#ifndef SDB_ELF_HPP
#define SDB_ELF_HPP

#include <filesystem>
#include <elf.h>

namespace sdb {
  class elf {
    public:
      elf(const std::filesystem::path& path);
      ~elf();

      elf(const elf&);
      elf& operator=(const elf&) = delete;

      std::filesystem::path path() const { return path_; }
      const Elf64_Ehdr& get_header() const { return header_; }

    private:
      int fd_;
      std::filesystem::path path_;
      std::size_t file_size_;
      std::byte* data_;
      Elf64_Ehdr header_;
  };
}

#endif
