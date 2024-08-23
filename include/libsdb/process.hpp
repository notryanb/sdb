#ifndef SDB_PROCESS_HPP
#define SDB_PROCESS_HPP

#include <filesystem>
#include <memory>
#include <cstdint>
#include <sys/types.h>

namespace sdb {
  enum class process_state {
    stopped,
    running,
    exited,
    terminated
  };

  struct stop_reason {
    stop_reason(int wait_status);

    process_state reason;
    std::uint8_t info;
  };

  class process {
    public:
      static std::unique_ptr<process> launch(std::filesystem::path path);
      static std::unique_ptr<process> attach(pid_t pid);

      process() = delete; // No default constructor
      process(const process&) = delete; // disable copy behavior
      process& operator=(const process&) = delete; // disable move behavior

     ~process(); 

      void resume();
      stop_reason wait_on_signal();

      pid_t pid() const { return pid_; }
      process_state state() const { return state_; }

    private:
      pid_t pid_ = 0;
      bool terminate_on_end_ = true;
      process_state state_ = process_state::stopped;

      process(pid_t pid, bool terminate_on_end) : pid_(pid), terminate_on_end_(terminate_on_end) {}
  };
}


#endif
