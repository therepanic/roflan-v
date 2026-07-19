#include "Vwriteback_wrapper.h"
#include "verilated.h"

#include <cstdint>
#include <cstdlib>
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

void drive(Vwriteback_wrapper &dut, std::uint32_t valid, std::uint32_t reg_we,
           std::uint32_t rd_addr, std::uint32_t wb_data) {
  dut.memory_valid = valid;
  dut.memory_reg_we = reg_we;
  dut.memory_rd_addr = rd_addr;
  dut.memory_wb_data = wb_data;
  dut.eval();
}

void check_enabled(Vwriteback_wrapper &dut, const std::string &name,
                   std::uint32_t rd_addr, std::uint32_t wb_data) {
  drive(dut, 1, 1, rd_addr, wb_data);
  check_eq(name, "reg_wr_en", 1, dut.reg_wr_en);
  check_eq(name, "reg_wr_addr", rd_addr, dut.reg_wr_addr);
  check_eq(name, "reg_wr_data", wb_data, dut.reg_wr_data);
}

void test_valid_writes(Vwriteback_wrapper &dut) {
  check_enabled(dut, "valid x1 zero", 1, 0x00000000u);
  check_enabled(dut, "valid x15 all ones", 15, 0xffffffffu);
  check_enabled(dut, "valid x31 sign bit", 31, 0x80000000u);
  check_enabled(dut, "valid x1 pattern", 1, 0x12345678u);
}

void test_disabled_writes(Vwriteback_wrapper &dut) {
  drive(dut, 0, 1, 23, 0xdeadbeefu);
  check_eq("invalid bubble", "reg_wr_en", 0, dut.reg_wr_en);

  drive(dut, 1, 0, 19, 0xcafebabeu);
  check_eq("reg_we disabled", "reg_wr_en", 0, dut.reg_wr_en);
}

void test_empty_packet(Vwriteback_wrapper &dut) {
  drive(dut, 0, 0, 0, 0);
  check_eq("empty packet", "reg_wr_en", 0, dut.reg_wr_en);
  check_eq("empty packet", "reg_wr_addr", 0, dut.reg_wr_addr);
  check_eq("empty packet", "reg_wr_data", 0, dut.reg_wr_data);
}

void check_current_outputs(Vwriteback_wrapper &dut, const std::string &name,
                           std::uint32_t expected_enable,
                           std::uint32_t expected_address,
                           std::uint32_t expected_data) {
  check_eq(name, "reg_wr_en", expected_enable, dut.reg_wr_en);
  check_eq(name, "reg_wr_addr", expected_address, dut.reg_wr_addr);
  check_eq(name, "reg_wr_data", expected_data, dut.reg_wr_data);
}

void test_combinational_switching(Vwriteback_wrapper &dut) {
  drive(dut, 1, 1, 5, 0x11112222u);
  check_current_outputs(dut, "switch valid A", 1, 5, 0x11112222u);

  drive(dut, 0, 1, 9, 0x33334444u);
  check_current_outputs(dut, "switch invalid", 0, 9, 0x33334444u);

  drive(dut, 1, 1, 17, 0x55556666u);
  check_current_outputs(dut, "switch valid B", 1, 17, 0x55556666u);

  drive(dut, 1, 0, 21, 0x77778888u);
  check_current_outputs(dut, "switch reg_we off", 0, 21, 0x77778888u);

  drive(dut, 1, 1, 3, 0x9999aaaau);
  check_current_outputs(dut, "switch valid again", 1, 3, 0x9999aaaau);
}

void test_x0_contract(Vwriteback_wrapper &dut) {
  check_enabled(dut, "writeback permits x0", 0, 0xabcdef01u);
}

void test_all_addresses(Vwriteback_wrapper &dut) {
  for (std::uint32_t address = 0; address < 32; ++address) {
    const std::uint32_t data = 0xa5a50000u | address;
    check_enabled(dut, "address " + std::to_string(address), address, data);
  }
}

} // namespace

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  Vwriteback_wrapper dut;
  test_valid_writes(dut);
  test_disabled_writes(dut);
  test_empty_packet(dut);
  test_combinational_switching(dut);
  test_x0_contract(dut);
  test_all_addresses(dut);
  dut.final();
  std::cout << checks << " writeback checks passed\n";
  return 0;
}
