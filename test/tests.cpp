#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <memory>
#include <sys/types.h>
#include <signal.h>
#include <libsdb/bit.hpp>
#include <libsdb/pipe.hpp>
#include <libsdb/process.hpp>
#include <libsdb/error.hpp>

using namespace sdb;

namespace {
  bool process_exists(pid_t pid) {
    auto ret = kill(pid, 0);
    return ret != -1 and errno != ESRCH;
  }

  char get_process_status(pid_t pid) {
    std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
    std::string data;
    std::getline(stat, data);
    auto index_of_last_parenthesis = data.rfind(')');
    auto index_of_status_indicator = index_of_last_parenthesis + 2;
    return data[index_of_status_indicator];
  }
}

TEST_CASE("Can create breakpoint site", "[breakpoint]") {
  auto proc = process::launch("targets/run_endlessly");
  auto& site = proc->create_breakpoint_site(virt_addr { 42 });
  REQUIRE(site.address().addr() == 42);
}

TEST_CASE("Breakpoint site ids increase", "[breakpoint]") {
  auto proc = process::launch("targets/run_endlessly");
  auto& s1 = proc->create_breakpoint_site(virt_addr { 42 });
  REQUIRE(s1.address().addr() == 42);

  auto& s2 = proc->create_breakpoint_site(virt_addr { 43 });
  REQUIRE(s2.id() == s1.id() + 1);

  auto& s3 = proc->create_breakpoint_site(virt_addr { 44 });
  REQUIRE(s3.id() == s1.id() + 2);
  
  auto& s4 = proc->create_breakpoint_site(virt_addr { 45 });
  REQUIRE(s4.id() == s1.id() + 3);
}

TEST_CASE("Read register works", "[register]") {
  auto proc = process::launch("targets/reg_read");
  auto& regs = proc->get_registers();

  proc->resume();
  proc->wait_on_signal();
  REQUIRE(regs.read_by_id_as<std::uint64_t>(register_id::r13) == 0xcafecafe);

  proc->resume();
  proc->wait_on_signal();
  REQUIRE(regs.read_by_id_as<std::uint8_t>(register_id::r13b) == 42);

  proc->resume();
  proc->wait_on_signal();
  REQUIRE(regs.read_by_id_as<byte64>(register_id::mm0) == to_byte64(0xba5eba11ull));

  proc->resume();
  proc->wait_on_signal();
  REQUIRE(regs.read_by_id_as<byte128>(register_id::xmm0) == to_byte128(64.125));

  proc->resume();
  proc->wait_on_signal();
  REQUIRE(regs.read_by_id_as<long double>(register_id::st0) == 64.125L);
}

TEST_CASE("Write register works", "[register]") {
  bool close_on_exec = false;
  sdb::pipe channel(close_on_exec);

  auto proc = process::launch("targets/reg_write", true, channel.get_write());

  channel.close_write();

  proc->resume();
  proc->wait_on_signal();

  auto& regs = proc->get_registers();

  regs.write_by_id(register_id::rsi, 0xcafecafe);
  proc->resume();
  proc->wait_on_signal();
  auto output  = channel.read();
  REQUIRE(to_string_view(output) == "0xcafecafe");

  regs.write_by_id(register_id::mm0, 0xba5eba11);
  proc->resume();
  proc->wait_on_signal();
  output = channel.read();
  REQUIRE(to_string_view(output) == "0xba5eba11");

  regs.write_by_id(register_id::xmm0, 42.24);
  proc->resume();
  proc->wait_on_signal();
  output = channel.read();
  REQUIRE(to_string_view(output) == "42.24");

  regs.write_by_id(register_id::st0, 42.24l);
  // The status word(16 bits wide) tracks the current size of the FPU stack
  // set bites 11..13 to 0b111
  regs.write_by_id(register_id::fsw, std::uint16_t{0b0011100000000000});
  // The 16 bit tag register tracks which 'st' register are valid.
  // 0b11 means empty and 0b00 means valid
  regs.write_by_id(register_id::ftw, std::uint16_t{0b0011111111111111});
  proc->resume();
  proc->wait_on_signal();
  output = channel.read();
  REQUIRE(to_string_view(output) == "42.24");
}

TEST_CASE("process::launch success", "[process]") {
  auto proc = process::launch("yes");
  REQUIRE(process_exists(proc->pid()));
}

TEST_CASE("process::launch no such program", "[process]") {
  REQUIRE_THROWS_AS(process::launch("you_do_not_have_to_be_good"), error);
}

/* NOTE: 
   In order for this test to pass, 
   the 'tests' exec must be run from within the build/test dir so the path is correct.
*/
TEST_CASE("process::attach success", "[process]") {
  auto target = process::launch("targets/run_endlessly", false);
  auto proc = process::attach(target->pid());
  REQUIRE(get_process_status(target->pid()) == 't');
}

TEST_CASE("process::attach invalid PID", "[process]") {
  REQUIRE_THROWS_AS(process::attach(0), error);
}

TEST_CASE("process::resume success", "[process]") {
  {
    auto proc = process::launch("targets/run_endlessly");
    proc->resume();
    auto status = get_process_status(proc->pid());
    auto success = status == 'R' or status == 'S';
    REQUIRE(success);
  }
  
  {
    auto target = process::launch("targets/run_endlessly", false);
    auto proc = process::attach(target->pid());
    proc->resume();
    auto status = get_process_status(proc->pid());
    auto success = status == 'R' or status == 'S';
    REQUIRE(success);
  }
}

TEST_CASE("process::resume already terminated", "[process]") {
  auto proc = process::launch("targets/end_immediately");
  proc->resume();
  proc->wait_on_signal();
  REQUIRE_THROWS_AS(proc->resume(), error);
}

