#include "Vfetch_wrapper.h"
#include "verilated.h"

#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

std::uint32_t checks = 0;

void check_eq(const std::string &test, const std::string &field,
              std::uint32_t expected, std::uint32_t actual) {
  if (expected != actual) {
    std::cerr << "FAIL " << test << '.' << field << ": expected=0x" << std::hex
              << std::setw(8) << std::setfill('0') << expected << " actual=0x"
              << std::setw(8) << actual << std::dec << '\n';
    std::exit(1);
  }
  ++checks;
}

struct FetchState {
  std::uint32_t valid;
  std::uint32_t instr;
  std::uint32_t pc;
  std::uint32_t cyc;
  std::uint32_t stb;
  std::uint32_t adr;
};

class Testbench {
public:
  Testbench() {
    dut_.clk = 0;
    dut_.rst = 0;
    dut_.decode_ready = 0;
    dut_.update_pc = 0;
    dut_.update_pc_target = 0;
    dut_.wb_dat_i = 0;
    dut_.wb_ack_i = 0;
    dut_.eval();
  }

  ~Testbench() { dut_.final(); }

  Vfetch_wrapper &dut() { return dut_; }

  FetchState state() const {
    return {static_cast<std::uint32_t>(dut_.fetch_valid),
            dut_.fetch_instr,
            dut_.fetch_pc,
            static_cast<std::uint32_t>(dut_.wb_cyc_o),
            static_cast<std::uint32_t>(dut_.wb_stb_o),
            dut_.wb_adr_o};
  }

  void rising(const std::string &name) {
    dut_.clk = 0;
    dut_.eval();
    const FetchState before = state();
    const bool ack = dut_.wb_ack_i != 0;
    const bool active = before.cyc != 0 && before.stb != 0;
    const bool consume = before.valid != 0 && dut_.decode_ready != 0;
    const bool redirect = dut_.update_pc != 0;
    const bool reset = dut_.rst != 0;

    if (ack) {
      check_eq(name, "ack_has_active_cycle", 1, active ? 1u : 0u);
    }

    dut_.clk = 1;
    dut_.eval();
    const FetchState after = state();

    if (!reset && !redirect && !ack && !consume) {
      check_eq(name, "ifid_valid_stable", before.valid, after.valid);
      check_eq(name, "ifid_instr_stable", before.instr, after.instr);
      check_eq(name, "ifid_pc_stable", before.pc, after.pc);
    }

    if (!reset && !ack) {
      check_eq(name, "no_ack_instr_stable", before.instr, after.instr);
      check_eq(name, "no_ack_pc_stable", before.pc, after.pc);
    }

    if (!reset && !redirect && active && !ack) {
      check_eq(name, "active_cycle_stable", 1, after.cyc);
      check_eq(name, "active_strobe_stable", 1, after.stb);
      check_eq(name, "active_address_stable", before.adr, after.adr);
    }

    if (!reset && !redirect && active && ack) {
      check_eq(name, "ack_sets_valid", 1, after.valid);
      check_eq(name, "ack_writes_once_instr", dut_.wb_dat_i, after.instr);
      check_eq(name, "ack_writes_once_pc", before.adr, after.pc);
    }

    dut_.clk = 0;
    dut_.eval();
  }

  void reset(const std::string &name) {
    dut_.wb_ack_i = 0;
    dut_.update_pc = 0;
    dut_.rst = 1;
    rising(name);
    check_eq(name, "fetch_valid_during_reset", 0, dut_.fetch_valid);
    check_eq(name, "fetch_ready_during_reset", 1, dut_.fetch_ready);
    check_eq(name, "wb_cyc_o_during_reset", 0, dut_.wb_cyc_o);
    check_eq(name, "wb_stb_o_during_reset", 0, dut_.wb_stb_o);
    check_eq(name, "wb_we_o_during_reset", 0, dut_.wb_we_o);
    check_eq(name, "wb_sel_o_during_reset", 0xf, dut_.wb_sel_o);
    check_eq(name, "wb_dat_o_during_reset", 0, dut_.wb_dat_o);
    dut_.rst = 0;
    dut_.eval();
  }

  void start_request(const std::string &name, std::uint32_t address) {
    rising(name);
    check_eq(name, "wb_cyc_o", 1, dut_.wb_cyc_o);
    check_eq(name, "wb_stb_o", 1, dut_.wb_stb_o);
    check_eq(name, "wb_adr_o", address, dut_.wb_adr_o);
  }

  void wait_without_ack(const std::string &name, unsigned cycles,
                        std::uint32_t address) {
    dut_.wb_ack_i = 0;
    for (unsigned cycle = 1; cycle <= cycles; ++cycle) {
      rising(name + " cycle " + std::to_string(cycle));
      check_eq(name, "wb_cyc_o", 1, dut_.wb_cyc_o);
      check_eq(name, "wb_stb_o", 1, dut_.wb_stb_o);
      check_eq(name, "wb_adr_o", address, dut_.wb_adr_o);
      check_eq(name, "fetch_valid", 0, dut_.fetch_valid);
    }
  }

  void acknowledge(const std::string &name, std::uint32_t data,
                   std::uint32_t address) {
    dut_.wb_dat_i = data;
    dut_.wb_ack_i = 1;
    rising(name);
    dut_.wb_ack_i = 0;
    dut_.eval();
    check_eq(name, "fetch_valid", 1, dut_.fetch_valid);
    check_eq(name, "fetch_instr", data, dut_.fetch_instr);
    check_eq(name, "fetch_pc", address, dut_.fetch_pc);
    check_eq(name, "wb_cyc_o", 0, dut_.wb_cyc_o);
    check_eq(name, "wb_stb_o", 0, dut_.wb_stb_o);
  }

private:
  Vfetch_wrapper dut_;
};

void test_reset_and_ack_delays() {
  for (const unsigned delay : {1u, 2u, 5u}) {
    Testbench tb;
    auto &dut = tb.dut();
    const std::string name = "ack delay " + std::to_string(delay);
    dut.decode_ready = 1;
    tb.reset(name + " reset");
    check_eq(name, "reset fetch_valid", 0, dut.fetch_valid);
    check_eq(name, "reset fetch_ready", 1, dut.fetch_ready);
    check_eq(name, "reset wb_cyc_o", 0, dut.wb_cyc_o);
    check_eq(name, "reset wb_stb_o", 0, dut.wb_stb_o);
    check_eq(name, "wb_we_o", 0, dut.wb_we_o);
    check_eq(name, "wb_sel_o", 0xf, dut.wb_sel_o);
    check_eq(name, "wb_dat_o", 0, dut.wb_dat_o);

    tb.start_request(name + " first request", 0x00000100u);
    tb.wait_without_ack(name + " wait", delay, 0x00000100u);
    tb.acknowledge(name + " response", 0x00500093u, 0x00000100u);
    tb.start_request(name + " sequential request", 0x00000104u);
  }
}

void test_decode_stall() {
  Testbench tb;
  auto &dut = tb.dut();
  dut.decode_ready = 0;
  tb.reset("decode stall reset");
  tb.start_request("decode stall request", 0x00000100u);
  tb.acknowledge("decode stall response", 0x00500093u, 0x00000100u);

  for (unsigned cycle = 1; cycle <= 3; ++cycle) {
    const std::string name = "decode stall cycle " + std::to_string(cycle);
    tb.rising(name);
    check_eq(name, "fetch_ready", 0, dut.fetch_ready);
    check_eq(name, "fetch_valid", 1, dut.fetch_valid);
    check_eq(name, "fetch_instr", 0x00500093u, dut.fetch_instr);
    check_eq(name, "fetch_pc", 0x00000100u, dut.fetch_pc);
    check_eq(name, "wb_cyc_o", 0, dut.wb_cyc_o);
    check_eq(name, "wb_stb_o", 0, dut.wb_stb_o);
  }

  dut.decode_ready = 1;
  dut.eval();
  check_eq("decode stall release", "fetch_ready", 1, dut.fetch_ready);
  tb.rising("decode stall release");
  check_eq("decode stall release", "old instruction consumed", 0,
           dut.fetch_valid);
  check_eq("decode stall release", "wb_cyc_o", 1, dut.wb_cyc_o);
  check_eq("decode stall release", "wb_stb_o", 1, dut.wb_stb_o);
  check_eq("decode stall release", "wb_adr_o", 0x00000104u, dut.wb_adr_o);
}

void test_empty_ifid_with_decode_stalled() {
  Testbench tb;
  auto &dut = tb.dut();
  dut.decode_ready = 0;
  tb.reset("empty IFID reset");
  check_eq("empty IFID", "fetch_valid", 0, dut.fetch_valid);
  check_eq("empty IFID", "fetch_ready", 1, dut.fetch_ready);
  tb.start_request("empty IFID request", 0x00000100u);
  tb.acknowledge("empty IFID fill", 0x00c00113u, 0x00000100u);
  check_eq("filled IFID", "fetch_ready", 0, dut.fetch_ready);

  for (unsigned cycle = 1; cycle <= 2; ++cycle) {
    const std::string name = "filled IFID stop " + std::to_string(cycle);
    tb.rising(name);
    check_eq(name, "fetch_valid", 1, dut.fetch_valid);
    check_eq(name, "fetch_instr", 0x00c00113u, dut.fetch_instr);
    check_eq(name, "fetch_pc", 0x00000100u, dut.fetch_pc);
    check_eq(name, "wb_cyc_o", 0, dut.wb_cyc_o);
  }
}

void test_simultaneous_consume_and_fetch() {
  Testbench tb;
  auto &dut = tb.dut();
  dut.decode_ready = 0;
  tb.reset("simultaneous reset");
  tb.start_request("simultaneous first request", 0x00000100u);
  tb.acknowledge("simultaneous first response", 0x00500093u, 0x00000100u);

  dut.decode_ready = 1;
  tb.rising("simultaneous consume and request");
  check_eq("simultaneous consume", "fetch_valid", 0, dut.fetch_valid);
  check_eq("simultaneous request", "wb_cyc_o", 1, dut.wb_cyc_o);
  check_eq("simultaneous request", "wb_adr_o", 0x00000104u, dut.wb_adr_o);

  dut.decode_ready = 0;
  tb.acknowledge("simultaneous second response", 0x00c00113u, 0x00000104u);
  tb.rising("simultaneous no duplicate");
  check_eq("simultaneous no duplicate", "fetch_valid", 1, dut.fetch_valid);
  check_eq("simultaneous no duplicate", "fetch_instr", 0x00c00113u,
           dut.fetch_instr);
  check_eq("simultaneous no duplicate", "fetch_pc", 0x00000104u, dut.fetch_pc);
  check_eq("simultaneous no duplicate", "wb_cyc_o", 0, dut.wb_cyc_o);
}

void test_redirect_without_request() {
  Testbench tb;
  auto &dut = tb.dut();
  dut.decode_ready = 0;
  tb.reset("redirect idle reset");
  tb.start_request("redirect idle old request", 0x00000100u);
  tb.acknowledge("redirect idle old response", 0x00500093u, 0x00000100u);
  check_eq("redirect idle setup", "fetch_valid", 1, dut.fetch_valid);
  check_eq("redirect idle setup", "wb_cyc_o", 0, dut.wb_cyc_o);
  dut.update_pc = 1;
  dut.update_pc_target = 0x00000200u;
  tb.rising("redirect idle pulse");
  check_eq("redirect idle pulse", "fetch_valid", 0, dut.fetch_valid);
  check_eq("redirect idle pulse", "wb_cyc_o", 0, dut.wb_cyc_o);
  check_eq("redirect idle pulse", "wb_stb_o", 0, dut.wb_stb_o);

  dut.update_pc = 0;
  tb.start_request("redirect idle target", 0x00000200u);
}

void test_redirect_active_request() {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset("redirect active reset");
  tb.start_request("redirect old request", 0x00000100u);
  dut.wb_dat_i = 0xdeadbeefu;
  dut.update_pc = 1;
  dut.update_pc_target = 0x00000300u;
  tb.rising("redirect cancels active");
  check_eq("redirect cancels active", "fetch_valid", 0, dut.fetch_valid);
  check_eq("redirect cancels active", "wb_cyc_o", 0, dut.wb_cyc_o);
  check_eq("redirect cancels active", "wb_stb_o", 0, dut.wb_stb_o);
  check_eq("redirect cancels active", "old data ignored", 0, dut.fetch_instr);

  dut.update_pc = 0;
  tb.start_request("redirect new request", 0x00000300u);
  tb.acknowledge("redirect new response", 0x01300193u, 0x00000300u);
  check_eq("redirect new response", "old data absent", 0x01300193u,
           dut.fetch_instr);
}

void test_redirect_held() {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset("held redirect reset");
  dut.update_pc = 1;
  dut.update_pc_target = 0x00000500u;

  for (unsigned cycle = 1; cycle <= 3; ++cycle) {
    const std::string name = "held redirect cycle " + std::to_string(cycle);
    tb.rising(name);
    check_eq(name, "fetch_valid", 0, dut.fetch_valid);
    check_eq(name, "wb_cyc_o", 0, dut.wb_cyc_o);
    check_eq(name, "wb_stb_o", 0, dut.wb_stb_o);
  }

  dut.update_pc = 0;
  tb.start_request("held redirect target", 0x00000500u);
}

void test_redirect_with_ack() {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset("redirect ACK reset");
  tb.start_request("redirect ACK old request", 0x00000100u);
  dut.wb_dat_i = 0xdeadbeefu;
  dut.wb_ack_i = 1;
  dut.update_pc = 1;
  dut.update_pc_target = 0x00000400u;
  tb.rising("redirect wins over ACK");
  check_eq("redirect wins over ACK", "fetch_valid", 0, dut.fetch_valid);
  check_eq("redirect wins over ACK", "wb_cyc_o", 0, dut.wb_cyc_o);
  check_eq("redirect wins over ACK", "wb_stb_o", 0, dut.wb_stb_o);
  check_eq("redirect wins over ACK", "old response ignored", 0,
           dut.fetch_instr);

  dut.wb_ack_i = 0;
  dut.update_pc = 0;
  tb.start_request("redirect ACK target", 0x00000400u);
  tb.acknowledge("redirect ACK new response", 0x01400213u, 0x00000400u);
}

void test_reset_active_request() {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset("active reset initial");
  tb.start_request("active reset request", 0x00000100u);
  dut.rst = 1;
  dut.eval();
  check_eq("synchronous reset before edge", "wb_cyc_o", 1, dut.wb_cyc_o);
  check_eq("synchronous reset before edge", "wb_stb_o", 1, dut.wb_stb_o);
  tb.rising("active request reset edge");
  check_eq("active request reset edge", "fetch_valid", 0, dut.fetch_valid);
  check_eq("active request reset edge", "wb_cyc_o", 0, dut.wb_cyc_o);
  check_eq("active request reset edge", "wb_stb_o", 0, dut.wb_stb_o);

  dut.rst = 0;
  tb.start_request("active reset restart", 0x00000100u);
}

} // namespace

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  test_reset_and_ack_delays();
  test_decode_stall();
  test_empty_ifid_with_decode_stalled();
  test_simultaneous_consume_and_fetch();
  test_redirect_without_request();
  test_redirect_active_request();
  test_redirect_held();
  test_redirect_with_ack();
  test_reset_active_request();
  std::cout << checks << " fetch checks passed\n";
  return 0;
}
