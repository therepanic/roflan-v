#include "Vmemory_wrapper.h"
#include "verilated.h"

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

enum LsuOp : std::uint32_t {
  LSU_NONE = 0,
  LSU_LOAD_BYTE = 1,
  LSU_LOAD_HALF = 2,
  LSU_LOAD_WORD = 3,
  LSU_LOAD_BYTE_U = 4,
  LSU_LOAD_HALF_U = 5,
  LSU_STORE_BYTE = 6,
  LSU_STORE_HALF = 7,
  LSU_STORE_WORD = 8,
};

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

struct State {
  std::uint32_t ready;
  std::uint32_t valid;
  std::uint32_t data;
  std::uint32_t rd_addr;
  std::uint32_t reg_we;
  std::uint32_t adr;
  std::uint32_t bus_data;
  std::uint32_t sel;
  std::uint32_t we;
  std::uint32_t cyc;
  std::uint32_t stb;
};

class Testbench {
public:
  Testbench() {
    dut_.clk = 0;
    dut_.rst = 0;
    dut_.wb_dat_i = 0;
    dut_.wb_ack_i = 0;
    clear_execute();
    dut_.eval();
  }

  ~Testbench() { dut_.final(); }

  Vmemory_wrapper &dut() { return dut_; }

  void clear_execute() {
    dut_.execute_valid = 0;
    dut_.execute_lsu_op = LSU_NONE;
    dut_.execute_result = 0;
    dut_.execute_store_data = 0;
    dut_.execute_rd_addr = 0;
    dut_.execute_reg_we = 0;
  }

  void set_execute(std::uint32_t lsu_op, std::uint32_t result,
                   std::uint32_t store_data, std::uint32_t rd_addr,
                   std::uint32_t reg_we) {
    dut_.execute_valid = 1;
    dut_.execute_lsu_op = lsu_op;
    dut_.execute_result = result;
    dut_.execute_store_data = store_data;
    dut_.execute_rd_addr = rd_addr;
    dut_.execute_reg_we = reg_we;
  }

  State state() const {
    return {static_cast<std::uint32_t>(dut_.memory_ready),
            static_cast<std::uint32_t>(dut_.writeback_valid),
            dut_.writeback_data,
            static_cast<std::uint32_t>(dut_.writeback_rd_addr),
            static_cast<std::uint32_t>(dut_.writeback_reg_we),
            dut_.wb_adr_o,
            dut_.wb_dat_o,
            static_cast<std::uint32_t>(dut_.wb_sel_o),
            static_cast<std::uint32_t>(dut_.wb_we_o),
            static_cast<std::uint32_t>(dut_.wb_cyc_o),
            static_cast<std::uint32_t>(dut_.wb_stb_o)};
  }

  void rising(const std::string &name) {
    dut_.clk = 0;
    dut_.eval();
    const State before = state();
    const bool reset = dut_.rst != 0;
    const bool ack = dut_.wb_ack_i != 0;
    const bool active = before.cyc != 0 && before.stb != 0;

    if (!reset && before.ready == 0) {
      check_eq(name, "busy_has_cycle", 1, active ? 1u : 0u);
    }

    dut_.clk = 1;
    dut_.eval();
    const State after = state();

    if (!reset && active && !ack) {
      check_eq(name, "waiting_cyc_stable", 1, after.cyc);
      check_eq(name, "waiting_stb_stable", 1, after.stb);
      check_eq(name, "waiting_address_stable", before.adr, after.adr);
      check_eq(name, "waiting_data_stable", before.bus_data, after.bus_data);
      check_eq(name, "waiting_sel_stable", before.sel, after.sel);
      check_eq(name, "waiting_we_stable", before.we, after.we);
    }

    if (after.ready == 0) {
      check_eq(name, "busy_cyc", 1, after.cyc);
      check_eq(name, "busy_stb", 1, after.stb);
    } else {
      check_eq(name, "idle_cyc", 0, after.cyc);
      check_eq(name, "idle_stb", 0, after.stb);
    }

    dut_.clk = 0;
    dut_.eval();
  }

  void reset(const std::string &name) {
    dut_.wb_ack_i = 0;
    dut_.rst = 1;
    rising(name);
    check_eq(name, "memory_ready", 1, dut_.memory_ready);
    check_eq(name, "writeback_valid", 0, dut_.writeback_valid);
    check_eq(name, "wb_cyc_o", 0, dut_.wb_cyc_o);
    check_eq(name, "wb_stb_o", 0, dut_.wb_stb_o);
    check_eq(name, "wb_we_o", 0, dut_.wb_we_o);
    check_eq(name, "wb_sel_o", 0, dut_.wb_sel_o);
    check_eq(name, "wb_adr_o", 0, dut_.wb_adr_o);
    check_eq(name, "wb_dat_o", 0, dut_.wb_dat_o);
    dut_.rst = 0;
    dut_.eval();
  }

  void capture_lsu(const std::string &name, std::uint32_t operation,
                   std::uint32_t address, std::uint32_t store_data,
                   std::uint32_t rd_addr, std::uint32_t reg_we) {
    check_eq(name, "ready_before_capture", 1, dut_.memory_ready);
    set_execute(operation, address, store_data, rd_addr, reg_we);
    dut_.wb_ack_i = 0;
    rising(name);
    check_eq(name, "memory_ready", 0, dut_.memory_ready);
    check_eq(name, "wb_cyc_o", 1, dut_.wb_cyc_o);
    check_eq(name, "wb_stb_o", 1, dut_.wb_stb_o);
    check_eq(name, "wb_adr_o", address, dut_.wb_adr_o);
  }

  void wait_without_ack(const std::string &name, unsigned cycles) {
    const State request = state();
    dut_.wb_ack_i = 0;
    for (unsigned cycle = 1; cycle <= cycles; ++cycle) {
      rising(name + " cycle " + std::to_string(cycle));
      check_eq(name, "memory_ready", 0, dut_.memory_ready);
      check_eq(name, "writeback_valid", 0, dut_.writeback_valid);
      check_eq(name, "wb_cyc_o", 1, dut_.wb_cyc_o);
      check_eq(name, "wb_stb_o", 1, dut_.wb_stb_o);
      check_eq(name, "wb_adr_o", request.adr, dut_.wb_adr_o);
      check_eq(name, "wb_dat_o", request.bus_data, dut_.wb_dat_o);
      check_eq(name, "wb_sel_o", request.sel, dut_.wb_sel_o);
      check_eq(name, "wb_we_o", request.we, dut_.wb_we_o);
    }
  }

  void acknowledge(const std::string &name, std::uint32_t bus_data,
                   std::uint32_t expected_data, std::uint32_t rd_addr,
                   std::uint32_t reg_we) {
    dut_.wb_dat_i = bus_data;
    dut_.wb_ack_i = 1;
    rising(name);
    dut_.wb_ack_i = 0;
    dut_.eval();
    check_eq(name, "memory_ready", 1, dut_.memory_ready);
    check_eq(name, "writeback_valid", 1, dut_.writeback_valid);
    check_eq(name, "writeback_data", expected_data, dut_.writeback_data);
    check_eq(name, "writeback_rd_addr", rd_addr, dut_.writeback_rd_addr);
    check_eq(name, "writeback_reg_we", reg_we, dut_.writeback_reg_we);
    check_eq(name, "wb_cyc_o", 0, dut_.wb_cyc_o);
    check_eq(name, "wb_stb_o", 0, dut_.wb_stb_o);
  }

private:
  Vmemory_wrapper dut_;
};

void test_non_lsu() {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset("non-LSU reset");
  tb.set_execute(LSU_NONE, 0x12345678u, 0, 7, 1);
  tb.rising("non-LSU single");
  check_eq("non-LSU single", "writeback_valid", 1, dut.writeback_valid);
  check_eq("non-LSU single", "writeback_data", 0x12345678u, dut.writeback_data);
  check_eq("non-LSU single", "writeback_rd_addr", 7, dut.writeback_rd_addr);
  check_eq("non-LSU single", "writeback_reg_we", 1, dut.writeback_reg_we);
  check_eq("non-LSU single", "memory_ready", 1, dut.memory_ready);
  check_eq("non-LSU single", "wb_cyc_o", 0, dut.wb_cyc_o);
  check_eq("non-LSU single", "wb_stb_o", 0, dut.wb_stb_o);

  tb.clear_execute();
  tb.rising("non-LSU clear");
  check_eq("non-LSU clear", "writeback_valid", 0, dut.writeback_valid);

  tb.set_execute(LSU_NONE, 0xaaaaaaaau, 0, 8, 1);
  tb.rising("non-LSU back-to-back A");
  check_eq("non-LSU back-to-back A", "writeback_valid", 1, dut.writeback_valid);
  check_eq("non-LSU back-to-back A", "writeback_data", 0xaaaaaaaau,
           dut.writeback_data);
  check_eq("non-LSU back-to-back A", "writeback_rd_addr", 8,
           dut.writeback_rd_addr);

  tb.set_execute(LSU_NONE, 0xbbbbbbbbu, 0, 9, 1);
  tb.rising("non-LSU back-to-back B");
  check_eq("non-LSU back-to-back B", "writeback_valid", 1, dut.writeback_valid);
  check_eq("non-LSU back-to-back B", "writeback_data", 0xbbbbbbbbu,
           dut.writeback_data);
  check_eq("non-LSU back-to-back B", "writeback_rd_addr", 9,
           dut.writeback_rd_addr);
  check_eq("non-LSU back-to-back B", "writeback_reg_we", 1,
           dut.writeback_reg_we);
  check_eq("non-LSU back-to-back B", "wb_cyc_o", 0, dut.wb_cyc_o);
}

void run_ack_delay(unsigned delay) {
  Testbench tb;
  auto &dut = tb.dut();
  const std::string name = "ACK delay " + std::to_string(delay);
  tb.reset(name + " reset");
  tb.capture_lsu(name + " capture", LSU_LOAD_WORD, 0x00002000u, 0, 10, 1);
  check_eq(name, "load wb_we_o", 0, dut.wb_we_o);
  check_eq(name, "load wb_sel_o", 0xf, dut.wb_sel_o);
  tb.clear_execute();
  tb.wait_without_ack(name + " wait", delay);
  tb.acknowledge(name + " ACK", 0x89abcdefu, 0x89abcdefu, 10, 1);
  tb.rising(name + " no duplicate");
  check_eq(name + " no duplicate", "writeback_valid", 0, dut.writeback_valid);
}

void test_ack_delays() {
  run_ack_delay(1);
  run_ack_delay(2);
  run_ack_delay(5);
}

void run_load(const std::string &name, std::uint32_t operation,
              std::uint32_t address, std::uint32_t expected_sel,
              std::uint32_t bus_data, std::uint32_t expected_data) {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset(name + " reset");
  tb.capture_lsu(name + " capture", operation, address, 0, 12, 1);
  check_eq(name, "wb_we_o", 0, dut.wb_we_o);
  check_eq(name, "wb_sel_o", expected_sel, dut.wb_sel_o);
  check_eq(name, "wb_adr_o", address, dut.wb_adr_o);
  tb.clear_execute();
  tb.acknowledge(name + " ACK", bus_data, expected_data, 12, 1);
  tb.rising(name + " no duplicate");
  check_eq(name + " no duplicate", "writeback_valid", 0, dut.writeback_valid);
}

void test_loads() {
  constexpr std::uint32_t byte_lanes = 0x83828180u;
  run_load("LB lane 0", LSU_LOAD_BYTE, 0x00003000u, 0x1, byte_lanes,
           0xffffff80u);
  run_load("LB lane 1", LSU_LOAD_BYTE, 0x00003001u, 0x2, byte_lanes,
           0xffffff81u);
  run_load("LB lane 2", LSU_LOAD_BYTE, 0x00003002u, 0x4, byte_lanes,
           0xffffff82u);
  run_load("LB lane 3", LSU_LOAD_BYTE, 0x00003003u, 0x8, byte_lanes,
           0xffffff83u);
  run_load("LBU lane 0", LSU_LOAD_BYTE_U, 0x00003100u, 0x1, byte_lanes,
           0x00000080u);
  run_load("LBU lane 1", LSU_LOAD_BYTE_U, 0x00003101u, 0x2, byte_lanes,
           0x00000081u);
  run_load("LBU lane 2", LSU_LOAD_BYTE_U, 0x00003102u, 0x4, byte_lanes,
           0x00000082u);
  run_load("LBU lane 3", LSU_LOAD_BYTE_U, 0x00003103u, 0x8, byte_lanes,
           0x00000083u);
  run_load("LH offset 0", LSU_LOAD_HALF, 0x00003200u, 0x3, 0x80028001u,
           0xffff8001u);
  run_load("LH offset 2", LSU_LOAD_HALF, 0x00003202u, 0xc, 0x80028001u,
           0xffff8002u);
  run_load("LHU offset 0", LSU_LOAD_HALF_U, 0x00003300u, 0x3, 0x80028001u,
           0x00008001u);
  run_load("LHU offset 2", LSU_LOAD_HALF_U, 0x00003302u, 0xc, 0x80028001u,
           0x00008002u);
  run_load("LW", LSU_LOAD_WORD, 0x00003400u, 0xf, 0xdeadbeefu, 0xdeadbeefu);
}

void run_store(const std::string &name, std::uint32_t operation,
               std::uint32_t address, std::uint32_t store_data,
               std::uint32_t expected_sel, std::uint32_t expected_bus_data) {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset(name + " reset");
  tb.capture_lsu(name + " capture", operation, address, store_data, 0, 0);
  check_eq(name, "wb_we_o", 1, dut.wb_we_o);
  check_eq(name, "wb_sel_o", expected_sel, dut.wb_sel_o);
  check_eq(name, "wb_dat_o", expected_bus_data, dut.wb_dat_o);
  check_eq(name, "wb_adr_o", address, dut.wb_adr_o);
  tb.clear_execute();
  tb.acknowledge(name + " ACK", 0xfeedfaceu, 0, 0, 0);
  tb.rising(name + " clear writeback");
  check_eq(name + " clear writeback", "writeback_valid", 0,
           dut.writeback_valid);
}

void test_stores() {
  constexpr std::uint32_t store_data = 0xa1b2c3d4u;
  run_store("SB lane 0", LSU_STORE_BYTE, 0x00004000u, store_data, 0x1,
            0x000000d4u);
  run_store("SB lane 1", LSU_STORE_BYTE, 0x00004001u, store_data, 0x2,
            0x0000d400u);
  run_store("SB lane 2", LSU_STORE_BYTE, 0x00004002u, store_data, 0x4,
            0x00d40000u);
  run_store("SB lane 3", LSU_STORE_BYTE, 0x00004003u, store_data, 0x8,
            0xd4000000u);
  run_store("SH offset 0", LSU_STORE_HALF, 0x00004100u, store_data, 0x3,
            0x0000c3d4u);
  run_store("SH offset 2", LSU_STORE_HALF, 0x00004102u, store_data, 0xc,
            0xc3d40000u);
  run_store("SW", LSU_STORE_WORD, 0x00004200u, store_data, 0xf, store_data);
}

void test_stalled_input_and_ack_priority() {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset("stalled input reset");
  tb.capture_lsu("request A", LSU_STORE_WORD, 0x00005000u, 0x11223344u, 0, 0);
  const State request_a = tb.state();

  tb.set_execute(LSU_NONE, 0xabcdef01u, 0, 17, 1);
  tb.wait_without_ack("instruction B stalled", 3);
  check_eq("instruction B stalled", "request A address", request_a.adr,
           dut.wb_adr_o);
  check_eq("instruction B stalled", "request A data", request_a.bus_data,
           dut.wb_dat_o);
  check_eq("instruction B stalled", "writeback_valid", 0, dut.writeback_valid);

  tb.acknowledge("ACK A has priority", 0, 0, 0, 0);
  check_eq("ACK A has priority", "B not accepted as data", 0,
           dut.writeback_data);
  check_eq("ACK A has priority", "B not accepted as rd", 0,
           dut.writeback_rd_addr);

  tb.rising("instruction B accepted after bubble");
  check_eq("instruction B accepted after bubble", "writeback_valid", 1,
           dut.writeback_valid);
  check_eq("instruction B accepted after bubble", "writeback_data", 0xabcdef01u,
           dut.writeback_data);
  check_eq("instruction B accepted after bubble", "writeback_rd_addr", 17,
           dut.writeback_rd_addr);
  check_eq("instruction B accepted after bubble", "writeback_reg_we", 1,
           dut.writeback_reg_we);
  check_eq("instruction B accepted after bubble", "wb_cyc_o", 0, dut.wb_cyc_o);
}

void test_ack_without_request() {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset("stray ACK reset");
  const State before = tb.state();
  dut.wb_dat_i = 0xffffffffu;
  dut.wb_ack_i = 1;
  tb.rising("stray ACK");
  dut.wb_ack_i = 0;
  dut.eval();
  check_eq("stray ACK", "writeback_valid", 0, dut.writeback_valid);
  check_eq("stray ACK", "writeback_data", before.data, dut.writeback_data);
  check_eq("stray ACK", "writeback_rd_addr", before.rd_addr,
           dut.writeback_rd_addr);
  check_eq("stray ACK", "writeback_reg_we", before.reg_we,
           dut.writeback_reg_we);
  check_eq("stray ACK", "memory_ready", 1, dut.memory_ready);
  check_eq("stray ACK", "wb_cyc_o", 0, dut.wb_cyc_o);
  check_eq("stray ACK", "wb_stb_o", 0, dut.wb_stb_o);
}

void test_reset_during_request() {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset("active reset initial");
  tb.capture_lsu("active reset request", LSU_LOAD_WORD, 0x00006000u, 0, 20, 1);
  dut.rst = 1;
  dut.eval();
  check_eq("synchronous reset before edge", "wb_cyc_o", 1, dut.wb_cyc_o);
  check_eq("synchronous reset before edge", "wb_stb_o", 1, dut.wb_stb_o);
  tb.rising("active request reset edge");
  check_eq("active request reset edge", "memory_ready", 1, dut.memory_ready);
  check_eq("active request reset edge", "writeback_valid", 0,
           dut.writeback_valid);
  check_eq("active request reset edge", "wb_cyc_o", 0, dut.wb_cyc_o);
  check_eq("active request reset edge", "wb_stb_o", 0, dut.wb_stb_o);
}

} // namespace

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  test_non_lsu();
  test_ack_delays();
  test_loads();
  test_stores();
  test_stalled_input_and_ack_priority();
  test_ack_without_request();
  test_reset_during_request();
  std::cout << checks << " memory checks passed\n";
  return 0;
}
