#include "Vcontrol_flow.h"
#include "verilated.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

enum BranchOp : std::uint8_t {
  BRANCH_NONE = 0,
  BRANCH_EQ = 1,
  BRANCH_NE = 2,
  BRANCH_LT = 3,
  BRANCH_GE = 4,
  BRANCH_LTU = 5,
  BRANCH_GEU = 6,
  JAL = 7,
  JAL_R = 8,
};

int failures = 0;
int checks = 0;

void check(Vcontrol_flow &dut, const std::string &name, BranchOp op,
           std::uint32_t rs1, std::uint32_t rs2, std::uint32_t pc,
           std::uint32_t imm, bool expected_update,
           std::uint32_t expected_target = 0) {
  dut.op = op;
  dut.rs1_data = rs1;
  dut.rs2_data = rs2;
  dut.pc = pc;
  dut.imm = imm;
  dut.eval();

  const bool actual_update = dut.update_pc != 0;
  const std::uint32_t actual_target = dut.update_pc_target;
  const bool target_wrong = expected_update && actual_target != expected_target;
  if (actual_update != expected_update || target_wrong) {
    ++failures;
    std::cerr << "FAIL " << name << ": expected update_pc=" << expected_update
              << " target=0x" << std::hex << std::setw(8) << std::setfill('0')
              << expected_target << " actual update_pc=" << std::dec
              << actual_update << " target=0x" << std::hex << std::setw(8)
              << actual_target << " rs1=0x" << std::setw(8) << rs1 << " rs2=0x"
              << std::setw(8) << rs2 << " pc=0x" << std::setw(8) << pc
              << " imm=0x" << std::setw(8) << imm << std::dec << '\n';
  } else {
    ++checks;
  }
}

} // namespace

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  Vcontrol_flow dut;
  constexpr std::uint32_t pc = 0x00001000u;
  constexpr std::uint32_t forward = 0x00000024u;
  constexpr std::uint32_t target = pc + forward;

  check(dut, "BEQ taken", BRANCH_EQ, 0x12345678u, 0x12345678u, pc, forward,
        true, target);
  check(dut, "BEQ not taken", BRANCH_EQ, 1u, 2u, pc, forward, false);
  check(dut, "BNE taken", BRANCH_NE, 1u, 2u, pc, forward, true, target);
  check(dut, "BNE not taken", BRANCH_NE, 2u, 2u, pc, forward, false);

  check(dut, "BLT taken negative", BRANCH_LT, 0xffffffffu, 1u, pc, forward,
        true, target);
  check(dut, "BLT not taken negative", BRANCH_LT, 0u, 0xffffffffu, pc, forward,
        false);
  check(dut, "BGE taken negative", BRANCH_GE, 0xffffffffu, 0xffffffffu, pc,
        forward, true, target);
  check(dut, "BGE not taken negative", BRANCH_GE, 0x80000000u, 0xffffffffu, pc,
        forward, false);

  check(dut, "BLTU taken", BRANCH_LTU, 0u, 0xffffffffu, pc, forward, true,
        target);
  check(dut, "BLTU not taken", BRANCH_LTU, 0xffffffffu, 0u, pc, forward, false);
  check(dut, "BGEU taken", BRANCH_GEU, 0xffffffffu, 0u, pc, forward, true,
        target);
  check(dut, "BGEU not taken", BRANCH_GEU, 0u, 0xffffffffu, pc, forward, false);

  check(dut, "JAL pc plus imm", JAL, 0xaaaaaaaau, 0x55555555u, pc, forward,
        true, target);
  check(dut, "JALR clears bit zero", JAL_R, 0x00002000u, 0u, 0xdeadbeefu, 5u,
        true, 0x00002004u);
  check(dut, "BRANCH_NONE", BRANCH_NONE, 1u, 1u, pc, forward, false);

  check(dut, "BEQ negative immediate", BRANCH_EQ, 7u, 7u, pc, 0xfffffff0u, true,
        0x00000ff0u);
  check(dut, "JAL negative immediate", JAL, 0u, 0u, pc, 0xfffffff0u, true,
        0x00000ff0u);
  check(dut, "JALR negative immediate", JAL_R, 0x00002003u, 0u, 0u, 0xfffffffcu,
        true, 0x00001ffeu);

  dut.final();
  if (failures != 0) {
    std::cerr << failures << " control-flow test(s) failed\n";
    return 1;
  }
  std::cout << checks << " control-flow checks passed\n";
  return 0;
}
