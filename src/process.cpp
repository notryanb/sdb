#include <libsdb/process.hpp>
#include <libsdb/error.hpp>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

sdb::stop_reason::stop_reason(int wait_status) {
  if (WIFEXITED(wait_status)) {
    reason = process_state::exited;
    info = WEXITSTATUS(wait_status);
  } else if (WIFSIGNALED(wait_status)) {
    reason = process_state::terminated;
    info = WTERMSIG(wait_status); 
  } else if (WIFSTOPPED(wait_status))  {
    reason = process_state::stopped;
    info = WSTOPSIG(wait_status); 
  }
}

std::unique_ptr<sdb::process>
sdb::process::launch(std::filesystem::path path) {
  pid_t pid;

  if ((pid = fork()) < 0) {
    error::send_errno("fork failed");
  }

  if (pid == 0) {
    if (ptrace(PT_TRACE_ME, 0, nullptr, 0) < 0) {
      error::send_errno("tracing failed");
    }

    if (execlp(path.c_str(), path.c_str(), nullptr) < 0) {
      error::send_errno("exec failed");
    }
  }

  std::unique_ptr<process> proc (new process(pid, /*terminate_on_end=*/true));
  proc->wait_on_signal();

  return proc;
}

sdb::stop_reason
sdb::process::wait_on_signal() {
  int wait_status;
  int options = 0;

  if (waitpid(pid_, &wait_status, options) < 0) {
    error::send_errno("waitpid failed");
  }

  stop_reason reason(wait_status);
  state_ = reason.reason;
  return reason;
}

std::unique_ptr<sdb::process>
sdb::process::attach(pid_t pid) {
  if (pid == 0) {
    error::send("Invalid PID");
  }

  if (ptrace(PT_ATTACHEXC, pid, nullptr, 0) < 0) {
    error::send_errno("Could not attach");
  }

  std::unique_ptr<process> proc (new process(pid, /*terminate_on_end=*/false));
  proc->wait_on_signal();

  return proc;
}

sdb::process::~process() {
  if (pid_ != 0) {
    int status;
    if (state_ == process_state::running) {
      kill(pid_, SIGSTOP);
      waitpid(pid_, &status, 0);
    }

    ptrace(PT_DETACH, pid_, nullptr, 0);
    kill(pid_, SIGCONT);

    if (terminate_on_end_) {
      kill(pid_, SIGKILL);
      waitpid(pid_, &status, 0);
    }
  }
}

void sdb::process::resume() {
  if (ptrace(PT_CONTINUE, pid_, nullptr, 0) < 0) {
    error::send_errno("Could not resume");
  }

  state_ = process_state::running;
}

