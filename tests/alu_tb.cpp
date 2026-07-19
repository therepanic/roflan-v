#include "Valu.h"
#include "verilated.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

enum AluOp : std::uint8_t {
  ALU_ADD = 1,
  ALU_SUB = 2,
  ALU_XOR = 3,
  ALU_OR = 4,
  ALU_AND = 5,
  ALU_SLL = 6,
  ALU_SRL = 7,
  ALU_SRA = 8,
  ALU_SLT = 9,
  ALU_SLTU = 10,
};

int failures = 0;
int checks = 0;

void check(Valu &dut, const std::string &name, AluOp op, std::uint32_t lhs,
           std::uint32_t rhs, std::uint32_t expected) {
  dut.op = op;
  dut.op_a = lhs;
  dut.op_b = rhs;
  dut.eval();

  const std::uint32_t actual = dut.result;
  if (actual != expected) {
    ++failures;
    std::cerr << "FAIL " << name << ": expected=0x" << std::hex << std::setw(8)
              << std::setfill('0') << expected << " actual=0x" << std::setw(8)
              << actual << " lhs=0x" << std::setw(8) << lhs << " rhs=0x"
              << std::setw(8) << rhs << std::dec << '\n';
  } else {
    ++checks;
  }
}

} // namespace

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  Valu dut;

  check(dut, "ADD ordinary", ALU_ADD, 0x12345678u, 0x11111111u, 0x23456789u);
  check(dut, "ADD overflow", ALU_ADD, 0xffffffffu, 0x00000001u, 0x00000000u);
  check(dut, "ADD signed boundary overflow", ALU_ADD, 0x7fffffffu, 0x00000001u,
        0x80000000u);
  check(dut, "SUB ordinary", ALU_SUB, 0x12345678u, 0x11111111u, 0x01234567u);
  check(dut, "SUB underflow", ALU_SUB, 0x00000000u, 0x00000001u, 0xffffffffu);

  check(dut, "XOR", ALU_XOR, 0x55aa55aau, 0x0ff00ff0u, 0x5a5a5a5au);
  check(dut, "OR", ALU_OR, 0x80000000u, 0x00000001u, 0x80000001u);
  check(dut, "AND", ALU_AND, 0xffffffffu, 0x7fffffffu, 0x7fffffffu);

  check(dut, "SLL shift 0", ALU_SLL, 0x80000001u, 0u, 0x80000001u);
  check(dut, "SLL shift 1", ALU_SLL, 0x40000001u, 1u, 0x80000002u);
  check(dut, "SLL shift 31", ALU_SLL, 0x00000001u, 31u, 0x80000000u);
  check(dut, "SRL shift 0", ALU_SRL, 0x80000001u, 0u, 0x80000001u);
  check(dut, "SRL shift 1", ALU_SRL, 0x80000001u, 1u, 0x40000000u);
  check(dut, "SRL shift 31", ALU_SRL, 0x80000000u, 31u, 0x00000001u);
  check(dut, "SRA shift 0", ALU_SRA, 0x80000001u, 0u, 0x80000001u);
  check(dut, "SRA shift 1", ALU_SRA, 0x80000001u, 1u, 0xc0000000u);
  check(dut, "SRA shift 31 negative", ALU_SRA, 0x80000000u, 31u, 0xffffffffu);
  check(dut, "SRA shift 31 positive", ALU_SRA, 0x7fffffffu, 31u, 0x00000000u);

  check(dut, "SLT positive true", ALU_SLT, 7u, 12u, 1u);
  check(dut, "SLT positive false", ALU_SLT, 12u, 7u, 0u);
  check(dut, "SLT negative less", ALU_SLT, 0xffffffffu, 0u, 1u);
  check(dut, "SLT signed minimum", ALU_SLT, 0x80000000u, 0x7fffffffu, 1u);
  check(dut, "SLTU true", ALU_SLTU, 0x7fffffffu, 0x80000000u, 1u);
  check(dut, "SLTU false", ALU_SLTU, 0xffffffffu, 0x00000000u, 0u);

  dut.final();
  if (failures != 0) {
    std::cerr << failures << " ALU test(s) failed\n";
    return 1;
  }
  std::cout << checks << " ALU checks passed\n";
  return 0;
}
