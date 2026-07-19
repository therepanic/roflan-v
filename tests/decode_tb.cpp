#include "Vdecode.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

enum LsuOp : std::uint32_t {
  LSU_LOAD_BYTE = 1,
  LSU_LOAD_HALF = 2,
  LSU_LOAD_WORD = 3,
  LSU_LOAD_BYTE_U = 4,
  LSU_LOAD_HALF_U = 5,
  LSU_STORE_BYTE = 6,
  LSU_STORE_HALF = 7,
  LSU_STORE_WORD = 8,
};

enum PcOp : std::uint32_t {
  PC_LUI = 1,
  PC_AUIPC = 2,
};

enum EnvOp : std::uint32_t {
  ENV_CALL = 1,
  ENV_BREAK = 2,
};

enum BranchOp : std::uint32_t {
  BRANCH_EQ = 1,
  BRANCH_NE = 2,
  BRANCH_LT = 3,
  BRANCH_GE = 4,
  BRANCH_LTU = 5,
  BRANCH_GEU = 6,
  JAL = 7,
  JAL_R = 8,
};

enum AluOp : std::uint32_t {
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

enum OpSource : std::uint32_t {
  OP_SRC_REG = 1,
  OP_SRC_IMM = 2,
};

constexpr std::uint32_t kPc = 0x10203040u;
constexpr std::uint32_t kRs1Data = 0xa5a55a5au;
constexpr std::uint32_t kRs2Data = 0x13579bdfu;
std::uint32_t checks = 0;

template <typename Wide>
std::uint32_t bits(const Wide &value, unsigned lsb, unsigned width) {
  const unsigned word = lsb / 32;
  const unsigned shift = lsb % 32;
  std::uint64_t joined = value[word];
  if (shift + width > 32) {
    joined |= static_cast<std::uint64_t>(value[word + 1]) << 32;
  }
  const std::uint64_t mask =
      width == 32 ? 0xffffffffull : ((1ull << width) - 1ull);
  return static_cast<std::uint32_t>((joined >> shift) & mask);
}

void check_eq(const std::string &test, const std::string &field,
              std::uint32_t expected, std::uint32_t actual) {
  if (actual != expected) {
    std::cerr << "FAIL " << test << '.' << field << ": expected=0x" << std::hex
              << std::setw(8) << std::setfill('0') << expected << " actual=0x"
              << std::setw(8) << actual << std::dec << '\n';
    std::exit(1);
  }
  ++checks;
}

std::uint32_t valid(const Vdecode &dut) {
  return bits(dut.decode_execute, 158, 1);
}

std::uint32_t lsu_op(const Vdecode &dut) {
  return bits(dut.decode_execute, 153, 5);
}

std::uint32_t alu_op(const Vdecode &dut) {
  return bits(dut.decode_execute, 149, 4);
}

std::uint32_t env_op(const Vdecode &dut) {
  return bits(dut.decode_execute, 146, 3);
}

std::uint32_t pc_op(const Vdecode &dut) {
  return bits(dut.decode_execute, 143, 3);
}

std::uint32_t branch_op(const Vdecode &dut) {
  return bits(dut.decode_execute, 139, 4);
}

std::uint32_t op_src_b(const Vdecode &dut) {
  return bits(dut.decode_execute, 136, 3);
}

std::uint32_t rs1_data(const Vdecode &dut) {
  return bits(dut.decode_execute, 104, 32);
}

std::uint32_t rs2_data(const Vdecode &dut) {
  return bits(dut.decode_execute, 72, 32);
}

std::uint32_t rd_addr(const Vdecode &dut) {
  return bits(dut.decode_execute, 67, 5);
}

std::uint32_t immediate(const Vdecode &dut) {
  return bits(dut.decode_execute, 35, 32);
}

std::uint32_t reg_we(const Vdecode &dut) {
  return bits(dut.decode_execute, 34, 1);
}

std::uint32_t mem_re(const Vdecode &dut) {
  return bits(dut.decode_execute, 33, 1);
}

std::uint32_t mem_we(const Vdecode &dut) {
  return bits(dut.decode_execute, 32, 1);
}

std::uint32_t pc(const Vdecode &dut) { return bits(dut.decode_execute, 0, 32); }

void set_fetch(Vdecode &dut, bool input_valid, std::uint32_t instr,
               std::uint32_t input_pc) {
  dut.fetch_decode[0] = input_pc;
  dut.fetch_decode[1] = instr;
  dut.fetch_decode[2] = input_valid ? 1u : 0u;
}

void tick(Vdecode &dut) {
  dut.clk = 0;
  dut.eval();
  dut.clk = 1;
  dut.eval();
  dut.clk = 0;
  dut.eval();
}

void reset(Vdecode &dut) {
  dut.flush = 0;
  dut.execute_ready = 0;
  set_fetch(dut, false, 0, 0);
  dut.rst = 1;
  tick(dut);
  dut.rst = 0;
  dut.eval();
}

void capture(Vdecode &dut, std::uint32_t instr) {
  dut.flush = 0;
  dut.execute_ready = 1;
  dut.reg_rs1_data = kRs1Data;
  dut.reg_rs2_data = kRs2Data;
  set_fetch(dut, true, instr, kPc);
  tick(dut);
}

void check_r(Vdecode &dut, const std::string &name, std::uint32_t instr,
             std::uint32_t expected_alu) {
  capture(dut, instr);
  check_eq(name, "valid", 1, valid(dut));
  check_eq(name, "alu_op", expected_alu, alu_op(dut));
  check_eq(name, "op_src_b", OP_SRC_REG, op_src_b(dut));
  check_eq(name, "reg_we", 1, reg_we(dut));
  check_eq(name, "rd_addr", 5, rd_addr(dut));
  check_eq(name, "reg_rs1_addr", 6, dut.reg_rs1_addr);
  check_eq(name, "reg_rs2_addr", 7, dut.reg_rs2_addr);
  check_eq(name, "rs1_data", kRs1Data, rs1_data(dut));
  check_eq(name, "rs2_data", kRs2Data, rs2_data(dut));
  check_eq(name, "pc", kPc, pc(dut));
}

void check_i(Vdecode &dut, const std::string &name, std::uint32_t instr,
             std::uint32_t expected_alu, std::uint32_t expected_imm) {
  capture(dut, instr);
  check_eq(name, "valid", 1, valid(dut));
  check_eq(name, "alu_op", expected_alu, alu_op(dut));
  check_eq(name, "op_src_b", OP_SRC_IMM, op_src_b(dut));
  check_eq(name, "reg_we", 1, reg_we(dut));
  check_eq(name, "rd_addr", 5, rd_addr(dut));
  check_eq(name, "reg_rs1_addr", 6, dut.reg_rs1_addr);
  check_eq(name, "rs1_data", kRs1Data, rs1_data(dut));
  check_eq(name, "imm", expected_imm, immediate(dut));
  check_eq(name, "pc", kPc, pc(dut));
}

void check_load(Vdecode &dut, const std::string &name, std::uint32_t instr,
                std::uint32_t expected_lsu, std::uint32_t expected_imm) {
  capture(dut, instr);
  check_eq(name, "valid", 1, valid(dut));
  check_eq(name, "lsu_op", expected_lsu, lsu_op(dut));
  check_eq(name, "mem_re", 1, mem_re(dut));
  check_eq(name, "reg_we", 1, reg_we(dut));
  check_eq(name, "reg_rs1_addr", 6, dut.reg_rs1_addr);
  check_eq(name, "rs1_data", kRs1Data, rs1_data(dut));
  check_eq(name, "rd_addr", 5, rd_addr(dut));
  check_eq(name, "imm", expected_imm, immediate(dut));
  check_eq(name, "pc", kPc, pc(dut));
}

void check_store(Vdecode &dut, const std::string &name, std::uint32_t instr,
                 std::uint32_t expected_lsu, std::uint32_t expected_imm) {
  capture(dut, instr);
  check_eq(name, "valid", 1, valid(dut));
  check_eq(name, "lsu_op", expected_lsu, lsu_op(dut));
  check_eq(name, "mem_we", 1, mem_we(dut));
  check_eq(name, "reg_rs1_addr", 6, dut.reg_rs1_addr);
  check_eq(name, "reg_rs2_addr", 7, dut.reg_rs2_addr);
  check_eq(name, "rs1_data", kRs1Data, rs1_data(dut));
  check_eq(name, "rs2_data", kRs2Data, rs2_data(dut));
  check_eq(name, "imm", expected_imm, immediate(dut));
  check_eq(name, "pc", kPc, pc(dut));
}

void check_branch(Vdecode &dut, const std::string &name, std::uint32_t instr,
                  std::uint32_t expected_branch, std::uint32_t expected_imm) {
  capture(dut, instr);
  check_eq(name, "valid", 1, valid(dut));
  check_eq(name, "branch_op", expected_branch, branch_op(dut));
  check_eq(name, "reg_rs1_addr", 6, dut.reg_rs1_addr);
  check_eq(name, "reg_rs2_addr", 7, dut.reg_rs2_addr);
  check_eq(name, "rs1_data", kRs1Data, rs1_data(dut));
  check_eq(name, "rs2_data", kRs2Data, rs2_data(dut));
  check_eq(name, "imm", expected_imm, immediate(dut));
  check_eq(name, "pc", kPc, pc(dut));
}

void check_jal(Vdecode &dut, const std::string &name, std::uint32_t instr,
               std::uint32_t expected_imm) {
  capture(dut, instr);
  check_eq(name, "valid", 1, valid(dut));
  check_eq(name, "branch_op", JAL, branch_op(dut));
  check_eq(name, "rd_addr", 5, rd_addr(dut));
  check_eq(name, "reg_we", 1, reg_we(dut));
  check_eq(name, "imm", expected_imm, immediate(dut));
  check_eq(name, "pc", kPc, pc(dut));
}

void check_jalr(Vdecode &dut, const std::string &name, std::uint32_t instr,
                std::uint32_t expected_imm) {
  capture(dut, instr);
  check_eq(name, "valid", 1, valid(dut));
  check_eq(name, "branch_op", JAL_R, branch_op(dut));
  check_eq(name, "rd_addr", 5, rd_addr(dut));
  check_eq(name, "reg_we", 1, reg_we(dut));
  check_eq(name, "reg_rs1_addr", 6, dut.reg_rs1_addr);
  check_eq(name, "rs1_data", kRs1Data, rs1_data(dut));
  check_eq(name, "imm", expected_imm, immediate(dut));
  check_eq(name, "pc", kPc, pc(dut));
}

void check_u(Vdecode &dut, const std::string &name, std::uint32_t instr,
             std::uint32_t expected_pc_op, std::uint32_t expected_imm) {
  capture(dut, instr);
  check_eq(name, "valid", 1, valid(dut));
  check_eq(name, "pc_op", expected_pc_op, pc_op(dut));
  check_eq(name, "rd_addr", 5, rd_addr(dut));
  check_eq(name, "reg_we", 1, reg_we(dut));
  check_eq(name, "imm", expected_imm, immediate(dut));
  check_eq(name, "pc", kPc, pc(dut));
}

void check_env(Vdecode &dut, const std::string &name, std::uint32_t instr,
               std::uint32_t expected_env) {
  capture(dut, instr);
  check_eq(name, "valid", 1, valid(dut));
  check_eq(name, "env_op", expected_env, env_op(dut));
  check_eq(name, "pc", kPc, pc(dut));
}

void check_invalid(Vdecode &dut, const std::string &name, std::uint32_t instr) {
  capture(dut, instr);
  check_eq(name, "valid", 0, valid(dut));
}

void check_pipeline(Vdecode &dut) {
  reset(dut);
  check_eq("reset", "valid", 0, valid(dut));
  check_eq("decode_ready invalid", "decode_ready", 1, dut.decode_ready);

  dut.execute_ready = 0;
  dut.reg_rs1_data = kRs1Data;
  dut.reg_rs2_data = kRs2Data;
  set_fetch(dut, true, 0x007302b3u, kPc);
  tick(dut);
  check_eq("capture rising edge", "valid", 1, valid(dut));
  check_eq("capture rising edge", "alu_op", ALU_ADD, alu_op(dut));
  check_eq("decode_ready stalled", "decode_ready", 0, dut.decode_ready);

  std::array<std::uint32_t, 5> held{};
  for (unsigned i = 0; i < held.size(); ++i) {
    held[i] = dut.decode_execute[i];
  }
  set_fetch(dut, true, 0x407302b3u, 0x55667788u);
  dut.reg_rs1_data = 0x11111111u;
  dut.reg_rs2_data = 0x22222222u;
  tick(dut);
  for (unsigned i = 0; i < held.size(); ++i) {
    check_eq("stall holds ID/EX", "word" + std::to_string(i), held[i],
             dut.decode_execute[i]);
  }

  dut.execute_ready = 1;
  dut.eval();
  check_eq("decode_ready execute ready", "decode_ready", 1, dut.decode_ready);
  tick(dut);
  check_eq("capture after stall", "valid", 1, valid(dut));
  check_eq("capture after stall", "alu_op", ALU_SUB, alu_op(dut));
  check_eq("capture after stall", "pc", 0x55667788u, pc(dut));

  set_fetch(dut, true, 0x007302b3u, kPc);
  dut.flush = 1;
  tick(dut);
  check_eq("flush priority", "valid", 0, valid(dut));

  dut.flush = 0;
  dut.execute_ready = 0;
  set_fetch(dut, false, 0xffffffffu, 0xffffffffu);
  tick(dut);
  check_eq("bubble", "valid", 0, valid(dut));
  check_eq("decode_ready bubble", "decode_ready", 1, dut.decode_ready);
}

} // namespace

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  Vdecode dut;
  reset(dut);

  check_r(dut, "add", 0x007302b3u, ALU_ADD);
  check_r(dut, "sub", 0x407302b3u, ALU_SUB);
  check_r(dut, "xor", 0x007342b3u, ALU_XOR);
  check_r(dut, "or", 0x007362b3u, ALU_OR);
  check_r(dut, "and", 0x007372b3u, ALU_AND);
  check_r(dut, "sll", 0x007312b3u, ALU_SLL);
  check_r(dut, "srl", 0x007352b3u, ALU_SRL);
  check_r(dut, "sra", 0x407352b3u, ALU_SRA);
  check_r(dut, "slt", 0x007322b3u, ALU_SLT);
  check_r(dut, "sltu", 0x007332b3u, ALU_SLTU);

  check_i(dut, "addi positive", 0x07b30293u, ALU_ADD, 0x0000007bu);
  check_i(dut, "addi negative", 0xff030293u, ALU_ADD, 0xfffffff0u);
  check_i(dut, "xori", 0x05534293u, ALU_XOR, 0x00000055u);
  check_i(dut, "ori", 0x12336293u, ALU_OR, 0x00000123u);
  check_i(dut, "andi", 0xfff37293u, ALU_AND, 0xffffffffu);
  check_i(dut, "slli", 0x01f31293u, ALU_SLL, 0x0000001fu);
  check_i(dut, "srli", 0x00135293u, ALU_SRL, 0x00000001u);
  check_i(dut, "srai", 0x41f35293u, ALU_SRA, 0x0000001fu);
  check_i(dut, "slti", 0xff832293u, ALU_SLT, 0xfffffff8u);
  check_i(dut, "sltiu", 0xfff33293u, ALU_SLTU, 0xffffffffu);
  check_i(dut, "addi imm 0x400", 0x40030293u, ALU_ADD, 0x00000400u);
  check_i(dut, "addi imm -2048", 0x80030293u, ALU_ADD, 0xfffff800u);

  check_load(dut, "lb", 0xff030283u, LSU_LOAD_BYTE, 0xfffffff0u);
  check_load(dut, "lh", 0x00c31283u, LSU_LOAD_HALF, 0x0000000cu);
  check_load(dut, "lw", 0x01032283u, LSU_LOAD_WORD, 0x00000010u);
  check_load(dut, "lbu", 0xfff34283u, LSU_LOAD_BYTE_U, 0xffffffffu);
  check_load(dut, "lhu", 0x02035283u, LSU_LOAD_HALF_U, 0x00000020u);

  check_store(dut, "sb", 0x00730a23u, LSU_STORE_BYTE, 0x00000014u);
  check_store(dut, "sh negative", 0xfe731823u, LSU_STORE_HALF, 0xfffffff0u);
  check_store(dut, "sw", 0x00732e23u, LSU_STORE_WORD, 0x0000001cu);

  check_branch(dut, "beq positive", 0x00730863u, BRANCH_EQ, 0x00000010u);
  check_branch(dut, "bne negative", 0xfe7318e3u, BRANCH_NE, 0xfffffff0u);
  check_branch(dut, "blt", 0x00734c63u, BRANCH_LT, 0x00000018u);
  check_branch(dut, "bge", 0xfe7354e3u, BRANCH_GE, 0xffffffe8u);
  check_branch(dut, "bltu", 0x02736063u, BRANCH_LTU, 0x00000020u);
  check_branch(dut, "bgeu", 0xfe7370e3u, BRANCH_GEU, 0xffffffe0u);

  check_jal(dut, "jal positive", 0x001002efu, 0x00000800u);
  check_jal(dut, "jal negative", 0x801ff2efu, 0xfffff800u);
  check_jalr(dut, "jalr positive", 0x014302e7u, 0x00000014u);
  check_jalr(dut, "jalr negative", 0xfec302e7u, 0xffffffecu);

  check_u(dut, "lui", 0xabcde2b7u, PC_LUI, 0x000abcdeu);
  check_u(dut, "auipc", 0x12345297u, PC_AUIPC, 0x00012345u);

  check_env(dut, "ecall", 0x00000073u, ENV_CALL);
  check_env(dut, "ebreak", 0x00100073u, ENV_BREAK);

  check_invalid(dut, "unknown opcode", 0xffffffffu);
  check_invalid(dut, "invalid funct3", 0x00033283u);
  check_invalid(dut, "invalid funct7", 0x027302b3u);
  check_invalid(dut, "jalr invalid funct3", 0x014312e7u);
  check_invalid(dut, "invalid SYSTEM", 0x00200073u);

  check_pipeline(dut);

  dut.final();
  std::cout << checks << " decode checks passed\n";
  return 0;
}
