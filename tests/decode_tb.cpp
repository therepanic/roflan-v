#include "Vdecode_wrapper.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

using Vdecode = Vdecode_wrapper;

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

enum class ProducerStage {
  IdEx,
  ExMem,
  Pending,
  Writeback,
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

void clear_hazard_inputs(Vdecode &dut) {
  dut.execute_valid = 0;
  dut.execute_lsu_op = 0;
  dut.execute_result = 0;
  dut.execute_store_data = 0;
  dut.execute_rd_addr = 0;
  dut.execute_reg_we = 0;
  dut.pending_valid = 0;
  dut.pending_lsu_op = 0;
  dut.pending_result = 0;
  dut.pending_store_data = 0;
  dut.pending_rd_addr = 0;
  dut.pending_reg_we = 0;
  dut.writeback_valid = 0;
  dut.writeback_data = 0;
  dut.writeback_rd_addr = 0;
  dut.writeback_reg_we = 0;
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
  clear_hazard_inputs(dut);
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

const char *stage_name(ProducerStage stage) {
  switch (stage) {
  case ProducerStage::IdEx:
    return "ID/EX";
  case ProducerStage::ExMem:
    return "EX/MEM";
  case ProducerStage::Pending:
    return "pending EX/MEM";
  case ProducerStage::Writeback:
    return "MEM/WB";
  }
  return "unknown";
}

void set_external_producer(Vdecode &dut, ProducerStage stage,
                           std::uint32_t producer_valid,
                           std::uint32_t producer_reg_we,
                           std::uint32_t producer_rd) {
  if (stage == ProducerStage::ExMem) {
    dut.execute_valid = producer_valid;
    dut.execute_reg_we = producer_reg_we;
    dut.execute_rd_addr = producer_rd;
  } else if (stage == ProducerStage::Pending) {
    dut.pending_valid = producer_valid;
    dut.pending_reg_we = producer_reg_we;
    dut.pending_rd_addr = producer_rd;
  } else if (stage == ProducerStage::Writeback) {
    dut.writeback_valid = producer_valid;
    dut.writeback_reg_we = producer_reg_we;
    dut.writeback_rd_addr = producer_rd;
  }
}

void prepare_standard_producer(Vdecode &dut, ProducerStage stage) {
  reset(dut);
  dut.execute_ready = 1;
  if (stage == ProducerStage::IdEx) {
    capture(dut, 0x00100293u);
    check_eq("prepare ID/EX producer", "valid", 1, valid(dut));
    check_eq("prepare ID/EX producer", "rd_addr", 5, rd_addr(dut));
    check_eq("prepare ID/EX producer", "reg_we", 1, reg_we(dut));
  } else {
    set_external_producer(dut, stage, 1, 1, 5);
  }
}

void clear_stage_producer(Vdecode &dut, ProducerStage stage) {
  if (stage != ProducerStage::IdEx) {
    set_external_producer(dut, stage, 0, 0, 0);
  }
}

void expect_stage_hazard(ProducerStage stage, const std::string &source,
                         std::uint32_t consumer_instr) {
  Vdecode dut;
  prepare_standard_producer(dut, stage);
  const std::string name = std::string(stage_name(stage)) + " " + source;
  set_fetch(dut, true, consumer_instr, kPc);
  dut.execute_ready = 1;
  dut.eval();
  check_eq(name, "decode_ready", 0, dut.decode_ready);
  tick(dut);
  check_eq(name, "bubble_valid", 0, valid(dut));
  check_eq(name, "consumer_still_valid", 1, dut.fetch_decode[2] & 1u);

  clear_stage_producer(dut, stage);
  dut.eval();
  check_eq(name + " released", "decode_ready", 1, dut.decode_ready);
  tick(dut);
  check_eq(name + " captured", "valid", 1, valid(dut));
  check_eq(name + " captured", "rd_addr", 10, rd_addr(dut));
  set_fetch(dut, false, 0, 0);
  tick(dut);
  check_eq(name + " captured once", "valid", 0, valid(dut));
  dut.final();
}

void test_hazard_stages_and_sources() {
  constexpr std::array<ProducerStage, 4> stages = {
      ProducerStage::IdEx, ProducerStage::ExMem, ProducerStage::Pending,
      ProducerStage::Writeback};
  for (const ProducerStage stage : stages) {
    expect_stage_hazard(stage, "rs1", 0x00628533u);
    expect_stage_hazard(stage, "rs2", 0x00530533u);
  }
}

void expect_usage_result(const std::string &name, std::uint32_t instr,
                         std::uint32_t expected_hazard) {
  Vdecode dut;
  prepare_standard_producer(dut, ProducerStage::ExMem);
  set_fetch(dut, true, instr, kPc);
  dut.execute_ready = 1;
  dut.eval();
  check_eq(name, "decode_ready", expected_hazard ? 0u : 1u, dut.decode_ready);
  tick(dut);
  check_eq(name, "idex_valid", expected_hazard ? 0u : 1u, valid(dut));
  dut.final();
}

void test_instruction_source_usage() {
  expect_usage_result("R-type rs1", 0x00628533u, 1);
  expect_usage_result("R-type rs2", 0x00530533u, 1);
  expect_usage_result("I-type rs1", 0x00128513u, 1);
  expect_usage_result("I-type immediate is not rs2", 0x00530513u, 0);
  expect_usage_result("load rs1", 0x0002a503u, 1);
  expect_usage_result("load immediate is not rs2", 0x00532503u, 0);
  expect_usage_result("store rs1", 0x0062a023u, 1);
  expect_usage_result("store rs2", 0x00532023u, 1);
  expect_usage_result("branch rs1", 0x00628063u, 1);
  expect_usage_result("branch rs2", 0x00530063u, 1);
  expect_usage_result("JALR rs1", 0x000280e7u, 1);
  expect_usage_result("JALR immediate is not rs2", 0x005300e7u, 0);
  expect_usage_result("JAL has no source", 0x000000efu, 0);
  expect_usage_result("LUI has no source", 0x12345537u, 0);
  expect_usage_result("AUIPC has no source", 0x12345517u, 0);
  expect_usage_result("ECALL has no source", 0x00000073u, 0);
  expect_usage_result("EBREAK has no source", 0x00100073u, 0);
}

void prepare_idex_variant(Vdecode &dut, const std::string &condition) {
  if (condition == "valid zero") {
    reset(dut);
    dut.execute_ready = 1;
  } else if (condition == "reg_we zero") {
    reset(dut);
    dut.execute_ready = 1;
    capture(dut, 0x00208063u);
  } else if (condition == "rd zero") {
    reset(dut);
    dut.execute_ready = 1;
    capture(dut, 0x00100013u);
  } else {
    reset(dut);
    dut.execute_ready = 1;
    capture(dut, 0x00100393u);
  }
}

void expect_no_hazard_condition(
    ProducerStage stage, const std::string &condition,
    std::uint32_t producer_valid, std::uint32_t producer_reg_we,
    std::uint32_t producer_rd, std::uint32_t consumer_instr,
    std::uint32_t fetch_valid, std::uint32_t expected_idex_valid) {
  Vdecode dut;
  if (stage == ProducerStage::IdEx) {
    prepare_idex_variant(dut, condition);
  } else {
    reset(dut);
    dut.execute_ready = 1;
    set_external_producer(dut, stage, producer_valid, producer_reg_we,
                          producer_rd);
  }
  const std::string name =
      std::string(stage_name(stage)) + " no hazard " + condition;
  set_fetch(dut, fetch_valid != 0, consumer_instr, kPc);
  dut.eval();
  check_eq(name, "decode_ready", 1, dut.decode_ready);
  tick(dut);
  check_eq(name, "idex_valid", expected_idex_valid, valid(dut));
  dut.final();
}

void test_no_hazard_conditions() {
  constexpr std::array<ProducerStage, 4> stages = {
      ProducerStage::IdEx, ProducerStage::ExMem, ProducerStage::Pending,
      ProducerStage::Writeback};
  for (const ProducerStage stage : stages) {
    expect_no_hazard_condition(stage, "valid zero", 0, 1, 5, 0x00628533u, 1, 1);
    expect_no_hazard_condition(stage, "reg_we zero", 1, 0, 5, 0x00628533u, 1,
                               1);
    expect_no_hazard_condition(stage, "rd zero", 1, 1, 0, 0x00628533u, 1, 1);
    expect_no_hazard_condition(stage, "rd mismatch", 1, 1, 7, 0x00628533u, 1,
                               1);
    expect_no_hazard_condition(stage, "consumer ignores immediate rs2", 1, 1, 5,
                               0x00530513u, 1, 1);
    expect_no_hazard_condition(stage, "fetch invalid", 1, 1, 5, 0x00628533u, 0,
                               0);
    expect_no_hazard_condition(stage, "invalid encoding", 1, 1, 5, 0xffffffffu,
                               1, 0);
  }
}

void check_idex_zero(const std::string &name, const Vdecode &dut) {
  for (unsigned word = 0; word < 5; ++word) {
    check_eq(name, "word" + std::to_string(word), 0, dut.decode_execute[word]);
  }
}

void test_hazard_vs_execute_stall() {
  Vdecode dut;
  prepare_standard_producer(dut, ProducerStage::IdEx);
  set_fetch(dut, true, 0x00628533u, kPc);
  dut.execute_ready = 1;
  dut.eval();
  check_eq("data hazard free execute", "decode_ready", 0, dut.decode_ready);
  tick(dut);
  check_idex_zero("data hazard inserts bubble", dut);

  prepare_standard_producer(dut, ProducerStage::IdEx);
  std::array<std::uint32_t, 5> held{};
  for (unsigned word = 0; word < held.size(); ++word) {
    held[word] = dut.decode_execute[word];
  }
  set_fetch(dut, true, 0x00628533u, kPc);
  dut.execute_ready = 0;
  dut.eval();
  check_eq("execute stall with hazard", "decode_ready", 0, dut.decode_ready);
  tick(dut);
  for (unsigned word = 0; word < held.size(); ++word) {
    check_eq("execute stall holds ID/EX", "word" + std::to_string(word),
             held[word], dut.decode_execute[word]);
  }
  dut.final();
}

void test_reset_and_flush_priority() {
  Vdecode dut;
  prepare_standard_producer(dut, ProducerStage::IdEx);
  dut.rst = 1;
  tick(dut);
  check_idex_zero("reset clears full ID/EX", dut);
  dut.rst = 0;

  prepare_standard_producer(dut, ProducerStage::IdEx);
  set_fetch(dut, true, 0x00628533u, kPc);
  dut.execute_ready = 1;
  dut.flush = 1;
  tick(dut);
  check_idex_zero("flush beats hazard", dut);

  reset(dut);
  set_fetch(dut, true, 0x00628533u, kPc);
  dut.execute_ready = 1;
  dut.flush = 1;
  tick(dut);
  check_idex_zero("flush beats capture", dut);
  dut.flush = 0;
  tick(dut);
  check_eq("pipeline resumes after flush", "valid", 1, valid(dut));
  check_eq("pipeline resumes after flush", "rd_addr", 10, rd_addr(dut));
  dut.final();
}

void test_load_stage_transition() {
  Vdecode dut;
  reset(dut);
  dut.execute_ready = 1;
  capture(dut, 0x0000a283u);
  check_eq("load in ID/EX", "valid", 1, valid(dut));
  check_eq("load in ID/EX", "rd_addr", 5, rd_addr(dut));
  set_fetch(dut, true, 0x00628533u, kPc);
  dut.eval();
  check_eq("load hazard ID/EX", "decode_ready", 0, dut.decode_ready);
  tick(dut);
  check_eq("load leaves ID/EX as bubble", "valid", 0, valid(dut));

  set_external_producer(dut, ProducerStage::ExMem, 1, 1, 5);
  dut.execute_lsu_op = LSU_LOAD_WORD;
  dut.eval();
  check_eq("load hazard EX/MEM", "decode_ready", 0, dut.decode_ready);
  tick(dut);
  check_eq("consumer waits at EX/MEM", "valid", 0, valid(dut));

  set_external_producer(dut, ProducerStage::ExMem, 0, 0, 0);
  set_external_producer(dut, ProducerStage::Pending, 1, 1, 5);
  dut.pending_lsu_op = LSU_LOAD_WORD;
  dut.eval();
  check_eq("load hazard pending", "decode_ready", 0, dut.decode_ready);
  tick(dut);
  check_eq("consumer waits at pending", "valid", 0, valid(dut));

  set_external_producer(dut, ProducerStage::Pending, 0, 0, 0);
  set_external_producer(dut, ProducerStage::Writeback, 1, 1, 5);
  dut.eval();
  check_eq("load hazard writeback", "decode_ready", 0, dut.decode_ready);
  tick(dut);
  check_eq("consumer waits at writeback", "valid", 0, valid(dut));

  set_external_producer(dut, ProducerStage::Writeback, 0, 0, 0);
  dut.eval();
  check_eq("load disappears after writeback", "decode_ready", 1,
           dut.decode_ready);
  tick(dut);
  check_eq("load consumer captured", "valid", 1, valid(dut));
  check_eq("load consumer captured", "rd_addr", 10, rd_addr(dut));
  set_fetch(dut, false, 0, 0);
  tick(dut);
  check_eq("load consumer captured once", "valid", 0, valid(dut));
  dut.final();
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

  test_hazard_stages_and_sources();
  test_instruction_source_usage();
  test_no_hazard_conditions();
  test_hazard_vs_execute_stall();
  test_reset_and_flush_priority();
  test_load_stage_transition();

  dut.final();
  std::cout << checks << " decode checks passed\n";
  return 0;
}
