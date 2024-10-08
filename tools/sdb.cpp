#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <signal.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <libsdb/libsdb.hpp>
#include <libsdb/process.hpp>
#include <libsdb/error.hpp>

namespace {

  void print_stop_reason(const sdb::process& process, sdb::stop_reason reason) {
    std::cout << "Process " << process.pid() << ' ';

    switch (reason.reason) {
      case sdb::process_state::exited:
        std::cout << "exited with status "
                  << static_cast<int>(reason.info);
        break;
      case sdb::process_state::terminated:
        std::cout << "terminated with signal "
                  << strsignal(reason.info);
                  //<< sigabbrev_np(reason.info);
        break;
      case sdb::process_state::stopped:
        std::cout << "stopped with signal "
                  << strsignal(reason.info);
                  //<< sigabbrev_np(reason.info);
        break;
      case sdb::process_state::running:
        break;
    }  

    std::cout << std::endl;
  }

  std::unique_ptr<sdb::process> attach(int argc, const char** argv) {
    if (argc == 3 && argv[1] == std::string_view("-p")) {
      pid_t pid = std::atoi(argv[2]);
      return sdb::process::attach(pid);
    } else {
      const char* program_path = argv[1];
      return sdb::process::launch(program_path);
    }
  }


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

  void handle_command(std::unique_ptr<sdb::process>& process, std::string_view line) {
    auto args = split(line, ' ');
    auto command = args[0];

    if (is_prefix(command, "continue")) {
      process->resume();
      auto reason = process->wait_on_signal();
      print_stop_reason(*process, reason);
    } else {
      std::cerr << "Unknown command\n";
    }
  }

  void main_loop(std::unique_ptr<sdb::process>& process) {
    char* line = nullptr;
    while ((line = readline("sdb> ")) != nullptr) {
      std::string line_str;

      // TODO - learn more about std::string_view
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
        try {
          handle_command(process, line_str);
        }
        catch (const sdb::error& err) {
          std::cout << err.what() << '\n';
        }
      }
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
}

int main(int argc, const char** argv) {
  if (argc == 1) {
    std::cerr << "No arguments given\n";
    return -1;
  }

  try {
    auto process = attach(argc, argv);
    main_loop(process);
  }
  catch (const sdb::error& err) {
    std::cout << err.what() << '\n';
  }
}
