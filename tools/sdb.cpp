#include <iostream>
#include <unistd.h>
#include <string_view>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <libsdb/libsdb.hpp>

namespace {
  std::vector<std::string> split(std::string_view str, char delimiter) {
    std::vector<std::string> out{};
    std::stringstream ss {std::string{str}};
    std::string item;

    while (std::getline(ss, item, delimiter)) {
      out.push_back(item);
    }

    return out;
  }

  bool is_prefix(std::string_view str, std::string_view of) {
    if (str.size() > of.size()) return false;

    return std::equal(str.begin(), str.end(), of.begin());
  }

  void resume(pid_t pid) {
    if (ptrace(PT_CONTINUE, pid, (caddr_t)1, 0) < 0) {
      std::cout << pid;
      std::cout << "\n";

      std::cerr << "Couldn't continue\n";
      std::exit(-1);
    }
  }

  void wait_on_signal(pid_t pid) {
    int wait_status;
    int options = 0;

    if (waitpid(pid, &wait_status, options) < 0) {
      std::cerr << "waitpid failed\n";
      std::exit(-1);
    }
  }

  void handle_command(pid_t pid, std::string_view line) {
    auto args = split(line, ' ');
    auto command = args[0];

    if (is_prefix(command, "continue")) {
      resume(pid);
      wait_on_signal(pid);
    } else {
      std::cerr << "Unknown command\n";

    }
  }

  pid_t attach(int argc, const char** argv) {
    pid_t pid = 0;
    // TODO: Learn more about C++ 17 std::string_view vs std::string
    if (argc == 3 && argv[1] == std::string_view("-p")) {
      pid = std::atoi(argv[2]);

      if (pid <= 0) {
        std::cerr << "Invalid pid\n";
        return -1;
      }

      //if (ptrace(PT_ATTACHEXC, pid, /*addr=*/nullptr, /*dat=*/nullptr) <0) {
      if (ptrace(PT_ATTACHEXC, pid, nullptr, 0) < 0) {
        // NOTE: Having issues attaching to non-child processes. ie. pass in pid with -p.
        std::perror("Could not attach");
        return -1;
      }
    } else {
      const char* program_path = argv[1];

      if ((pid = fork()) < 0) {
        std::perror("fork failed");
        return -1;
      }

      if (pid == 0) {
        // We're in the child process
        // execute debugee
        if (ptrace(PT_TRACE_ME, 0, nullptr, 0) < 0) {
          std::perror("Tracing failed");
          return -1;
        }

        if (execlp(program_path, program_path, nullptr) < 0) {
          std::perror("Exec failed");
          return -1;
        }
      }
    }

    return pid;
  }
}

int main(int argc, const char** argv) {
  if (argc == 1) {
    std::cerr << "No arguments given\n";
    return -1;
  }

  pid_t pid = attach(argc, argv);

  if (pid < 0) {
    return 1;
  }

  int wait_status;
  int options = 0;

  if (waitpid(pid, &wait_status, options) < 0) {
    std::perror("waitpid failed");
  }

  char* line = nullptr;
  while ((line = readline("sdb> ")) != nullptr) {
    std::string line_str;

    if (line == std::string_view("")) {
      free(line);
      if (history_length > 0) {
        line_str = history_list()[history_length - 1]->line;
      }
    } else {
      line_str = line;
      add_history(line);
      free(line);
    }

    if (!line_str.empty()) {
      handle_command(pid, line_str);
    }
  }
}
