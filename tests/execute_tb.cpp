#include "Vexecute_wrapper.h"
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

enum PcOp : std::uint32_t {
  PC_NONE = 0,
  PC_LUI = 1,
  PC_AUIPC = 2,
};

enum BranchOp : std::uint32_t {
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

enum AluOp : std::uint32_t {
  ALU_NONE = 0,
  ALU_ADD = 1,
  ALU_SUB = 2,
  ALU_XOR = 3,
  ALU_OR = 4,
};

enum OpSource : std::uint32_t {
  OP_SRC_NONE = 0,
  OP_SRC_REG = 1,
  OP_SRC_IMM = 2,
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

struct MemoryState {
  std::uint32_t valid;
  std::uint32_t lsu_op;
  std::uint32_t result;
  std::uint32_t store_data;
  std::uint32_t rd_addr;
  std::uint32_t reg_we;
};

class Testbench {
public:
  Testbench() {
    dut_.clk = 0;
    dut_.rst = 0;
    dut_.memory_ready = 0;
    dut_.alu_result = 0;
    dut_.branch_update_pc = 0;
    dut_.branch_update_pc_target = 0;
    clear_decode();
    dut_.eval();
  }

  ~Testbench() { dut_.final(); }

  Vexecute_wrapper &dut() { return dut_; }

  void clear_decode() {
    dut_.decode_valid = 0;
    dut_.decode_lsu_op = LSU_NONE;
    dut_.decode_alu_op = ALU_NONE;
    dut_.decode_env_op = 0;
    dut_.decode_pc_op = PC_NONE;
    dut_.decode_branch_op = BRANCH_NONE;
    dut_.decode_op_src_b = OP_SRC_NONE;
    dut_.decode_rs1_data = 0;
    dut_.decode_rs2_data = 0;
    dut_.decode_rd_addr = 0;
    dut_.decode_imm = 0;
    dut_.decode_reg_we = 0;
    dut_.decode_mem_re = 0;
    dut_.decode_mem_we = 0;
    dut_.decode_pc = 0;
  }

  void rising() {
    dut_.clk = 0;
    dut_.eval();
    dut_.clk = 1;
    dut_.eval();
    dut_.clk = 0;
    dut_.eval();
  }

  void reset() {
    dut_.rst = 1;
    rising();
    dut_.rst = 0;
    dut_.eval();
  }

  MemoryState memory_state() const {
    return {static_cast<std::uint32_t>(dut_.memory_valid),
            static_cast<std::uint32_t>(dut_.memory_lsu_op),
            dut_.memory_result,
            dut_.memory_store_data,
            static_cast<std::uint32_t>(dut_.memory_rd_addr),
            static_cast<std::uint32_t>(dut_.memory_reg_we)};
  }

private:
  Vexecute_wrapper dut_;
};

void check_memory_held(const std::string &name, const MemoryState &expected,
                       const Vexecute_wrapper &dut) {
  check_eq(name, "valid", expected.valid, dut.memory_valid);
  check_eq(name, "lsu_op", expected.lsu_op, dut.memory_lsu_op);
  check_eq(name, "result", expected.result, dut.memory_result);
  check_eq(name, "store_data", expected.store_data, dut.memory_store_data);
  check_eq(name, "rd_addr", expected.rd_addr, dut.memory_rd_addr);
  check_eq(name, "reg_we", expected.reg_we, dut.memory_reg_we);
}

void test_reset_ready_stall_and_bubble() {
  Testbench tb;
  auto &dut = tb.dut();
  tb.reset();
  check_eq("reset", "memory_valid", 0, dut.memory_valid);
  check_eq("empty EX/MEM", "execute_ready", 1, dut.execute_ready);

  dut.decode_valid = 1;
  dut.decode_alu_op = ALU_XOR;
  dut.decode_op_src_b = OP_SRC_REG;
  dut.decode_rs1_data = 0x11111111u;
  dut.decode_rs2_data = 0x22222222u;
  dut.decode_rd_addr = 3;
  dut.decode_reg_we = 1;
  dut.alu_result = 0x33333333u;
  dut.memory_ready = 0;
  tb.rising();
  check_eq("fill EX/MEM", "memory_valid", 1, dut.memory_valid);
  check_eq("stalled EX/MEM", "execute_ready", 0, dut.execute_ready);
  const MemoryState held = tb.memory_state();

  dut.decode_alu_op = ALU_SUB;
  dut.decode_rs1_data = 0xaaaaaaaau;
  dut.decode_rs2_data = 0x55555555u;
  dut.decode_rd_addr = 9;
  dut.alu_result = 0x44444444u;
  tb.rising();
  check_memory_held("stall holds EX/MEM", held, dut);

  dut.memory_ready = 1;
  dut.eval();
  check_eq("release stall", "execute_ready", 1, dut.execute_ready);
  tb.rising();
  check_eq("capture after stall", "valid", 1, dut.memory_valid);
  check_eq("capture after stall", "result", 0x44444444u, dut.memory_result);
  check_eq("capture after stall", "rd_addr", 9, dut.memory_rd_addr);

  tb.clear_decode();
  tb.rising();
  check_eq("bubble", "memory_valid", 0, dut.memory_valid);
}

void check_alu(Vexecute_wrapper &dut, Testbench &tb, const std::string &name,
               std::uint32_t op, std::uint32_t source, std::uint32_t expected_b,
               std::uint32_t supplied_result, std::uint32_t rd) {
  tb.clear_decode();
  dut.decode_valid = 1;
  dut.decode_alu_op = op;
  dut.decode_op_src_b = source;
  dut.decode_rs1_data = 0x10203040u;
  dut.decode_rs2_data = 0x55667788u;
  dut.decode_imm = 0xffffffe0u;
  dut.decode_rd_addr = rd;
  dut.decode_reg_we = 1;
  dut.alu_result = supplied_result;
  dut.memory_ready = 1;
  dut.eval();
  check_eq(name, "alu_op", op, dut.alu_op);
  check_eq(name, "alu_op_a", 0x10203040u, dut.alu_op_a);
  check_eq(name, "alu_op_b", expected_b, dut.alu_op_b);
  tb.rising();
  check_eq(name, "result", supplied_result, dut.memory_result);
  check_eq(name, "rd_addr", rd, dut.memory_rd_addr);
  check_eq(name, "reg_we", 1, dut.memory_reg_we);
  check_eq(name, "lsu_op", LSU_NONE, dut.memory_lsu_op);
  check_eq(name, "valid", 1, dut.memory_valid);
}

void test_alu_paths() {
  Testbench tb;
  tb.reset();
  auto &dut = tb.dut();
  check_alu(dut, tb, "ALU register source", ALU_XOR, OP_SRC_REG, 0x55667788u,
            0xcafebabeu, 11);
  check_alu(dut, tb, "ALU immediate source", ALU_OR, OP_SRC_IMM, 0xffffffe0u,
            0x89abcdefu, 12);
}

void check_load(Vexecute_wrapper &dut, Testbench &tb, const std::string &name,
                std::uint32_t operation, std::uint32_t offset,
                std::uint32_t supplied_result, std::uint32_t rd) {
  tb.clear_decode();
  dut.decode_valid = 1;
  dut.decode_lsu_op = operation;
  dut.decode_rs1_data = 0x20001000u;
  dut.decode_imm = offset;
  dut.decode_rd_addr = rd;
  dut.decode_reg_we = 1;
  dut.alu_result = supplied_result;
  dut.memory_ready = 1;
  dut.eval();
  check_eq(name, "alu_op", ALU_ADD, dut.alu_op);
  check_eq(name, "alu_op_a", 0x20001000u, dut.alu_op_a);
  check_eq(name, "alu_op_b", offset, dut.alu_op_b);
  tb.rising();
  check_eq(name, "result", supplied_result, dut.memory_result);
  check_eq(name, "lsu_op", operation, dut.memory_lsu_op);
  check_eq(name, "rd_addr", rd, dut.memory_rd_addr);
  check_eq(name, "reg_we", 1, dut.memory_reg_we);
  check_eq(name, "valid", 1, dut.memory_valid);
}

void test_loads() {
  Testbench tb;
  tb.reset();
  auto &dut = tb.dut();
  check_load(dut, tb, "LB positive", LSU_LOAD_BYTE, 0x00000014u, 0x30000014u,
             1);
  check_load(dut, tb, "LH negative", LSU_LOAD_HALF, 0xfffffff0u, 0x30000020u,
             2);
  check_load(dut, tb, "LW positive", LSU_LOAD_WORD, 0x00000024u, 0x30000024u,
             3);
  check_load(dut, tb, "LBU negative", LSU_LOAD_BYTE_U, 0xffffffe0u, 0x30000030u,
             4);
  check_load(dut, tb, "LHU positive", LSU_LOAD_HALF_U, 0x00000034u, 0x30000034u,
             5);
}

void check_store(Vexecute_wrapper &dut, Testbench &tb, const std::string &name,
                 std::uint32_t operation, std::uint32_t offset,
                 std::uint32_t supplied_result, std::uint32_t store_value) {
  tb.clear_decode();
  dut.decode_valid = 1;
  dut.decode_lsu_op = operation;
  dut.decode_rs1_data = 0x40001000u;
  dut.decode_rs2_data = store_value;
  dut.decode_imm = offset;
  dut.decode_reg_we = 0;
  dut.alu_result = supplied_result;
  dut.memory_ready = 1;
  dut.eval();
  check_eq(name, "alu_op", ALU_ADD, dut.alu_op);
  check_eq(name, "alu_op_a", 0x40001000u, dut.alu_op_a);
  check_eq(name, "alu_op_b", offset, dut.alu_op_b);
  tb.rising();
  check_eq(name, "result", supplied_result, dut.memory_result);
  check_eq(name, "store_data", store_value, dut.memory_store_data);
  check_eq(name, "lsu_op", operation, dut.memory_lsu_op);
  check_eq(name, "reg_we", 0, dut.memory_reg_we);
  check_eq(name, "valid", 1, dut.memory_valid);
}

void test_stores() {
  Testbench tb;
  tb.reset();
  auto &dut = tb.dut();
  check_store(dut, tb, "SB", LSU_STORE_BYTE, 0x00000008u, 0x40001008u,
              0x11223344u);
  check_store(dut, tb, "SH", LSU_STORE_HALF, 0xfffffffcu, 0x40000ffcu,
              0x55667788u);
  check_store(dut, tb, "SW", LSU_STORE_WORD, 0x00000010u, 0x40001010u,
              0x99aabbccu);
}

void check_branch(Vexecute_wrapper &dut, Testbench &tb, const std::string &name,
                  std::uint32_t operation, std::uint32_t offset) {
  tb.clear_decode();
  dut.decode_valid = 1;
  dut.decode_branch_op = operation;
  dut.decode_rs1_data = 0x80000000u;
  dut.decode_rs2_data = 0x00000001u;
  dut.decode_pc = 0x00001000u;
  dut.decode_imm = offset;
  dut.decode_reg_we = 0;
  dut.branch_update_pc = 0;
  dut.memory_ready = 1;
  dut.eval();
  check_eq(name, "branch_op", operation, dut.branch_op);
  check_eq(name, "branch_rs1_data", 0x80000000u, dut.branch_rs1_data);
  check_eq(name, "branch_rs2_data", 0x00000001u, dut.branch_rs2_data);
  check_eq(name, "branch_pc", 0x00001000u, dut.branch_pc);
  check_eq(name, "branch_imm", offset, dut.branch_imm);
  tb.rising();
  check_eq(name, "memory_valid", 1, dut.memory_valid);
  check_eq(name, "memory_reg_we", 0, dut.memory_reg_we);
}

void test_conditional_branches() {
  Testbench tb;
  tb.reset();
  auto &dut = tb.dut();
  check_branch(dut, tb, "BEQ", BRANCH_EQ, 0x00000010u);
  check_branch(dut, tb, "BNE", BRANCH_NE, 0xfffffff0u);
  check_branch(dut, tb, "BLT", BRANCH_LT, 0x00000018u);
  check_branch(dut, tb, "BGE", BRANCH_GE, 0xffffffe8u);
  check_branch(dut, tb, "BLTU", BRANCH_LTU, 0x00000020u);
  check_branch(dut, tb, "BGEU", BRANCH_GEU, 0xffffffe0u);
}

void check_jump(Vexecute_wrapper &dut, Testbench &tb, const std::string &name,
                std::uint32_t operation, std::uint32_t jump_pc,
                std::uint32_t expected_result, std::uint32_t rd) {
  tb.clear_decode();
  dut.decode_valid = 1;
  dut.decode_branch_op = operation;
  dut.decode_rs1_data = 0x12345679u;
  dut.decode_rs2_data = 0xaaaaaaaau;
  dut.decode_pc = jump_pc;
  dut.decode_imm = 0xffffffecu;
  dut.decode_rd_addr = rd;
  dut.decode_reg_we = 1;
  dut.branch_update_pc = 0;
  dut.memory_ready = 1;
  dut.eval();
  check_eq(name, "branch_op", operation, dut.branch_op);
  check_eq(name, "branch_rs1_data", operation == JAL_R ? 0x12345679u : 0u,
           dut.branch_rs1_data);
  check_eq(name, "branch_rs2_data", 0, dut.branch_rs2_data);
  check_eq(name, "branch_pc", jump_pc, dut.branch_pc);
  check_eq(name, "branch_imm", 0xffffffecu, dut.branch_imm);
  tb.rising();
  check_eq(name, "result", expected_result, dut.memory_result);
  check_eq(name, "rd_addr", rd, dut.memory_rd_addr);
  check_eq(name, "reg_we", 1, dut.memory_reg_we);
  check_eq(name, "valid", 1, dut.memory_valid);
}

void test_jumps() {
  Testbench tb;
  tb.reset();
  auto &dut = tb.dut();
  check_jump(dut, tb, "JAL", JAL, 0x00002000u, 0x00002004u, 7);
  check_jump(dut, tb, "JALR wraparound", JAL_R, 0xfffffffcu, 0x00000000u, 8);
}

void check_pc_op(Vexecute_wrapper &dut, Testbench &tb, const std::string &name,
                 std::uint32_t operation, std::uint32_t instruction_pc,
                 std::uint32_t immediate, std::uint32_t expected_result,
                 std::uint32_t rd) {
  tb.clear_decode();
  dut.decode_valid = 1;
  dut.decode_pc_op = operation;
  dut.decode_pc = instruction_pc;
  dut.decode_imm = immediate;
  dut.decode_rd_addr = rd;
  dut.decode_reg_we = 1;
  dut.memory_ready = 1;
  dut.eval();
  tb.rising();
  check_eq(name, "result", expected_result, dut.memory_result);
  check_eq(name, "rd_addr", rd, dut.memory_rd_addr);
  check_eq(name, "reg_we", 1, dut.memory_reg_we);
  check_eq(name, "valid", 1, dut.memory_valid);
}

void test_lui_auipc() {
  Testbench tb;
  tb.reset();
  auto &dut = tb.dut();
  check_pc_op(dut, tb, "LUI", PC_LUI, 0x76543210u, 0x000abcdeu, 0xabcde000u,
              13);
  check_pc_op(dut, tb, "AUIPC wraparound", PC_AUIPC, 0xfffff000u, 0x00000002u,
              0x00001000u, 14);
}

void test_redirect_formula() {
  Testbench tb;
  tb.reset();
  auto &dut = tb.dut();
  dut.memory_ready = 1;
  dut.decode_valid = 1;
  dut.decode_branch_op = BRANCH_EQ;
  dut.branch_update_pc_target = 0x00004000u;
  dut.branch_update_pc = 1;
  dut.eval();
  check_eq("redirect all true", "execute_ready", 1, dut.execute_ready);
  check_eq("redirect all true", "update_pc", 1, dut.update_pc);
  check_eq("redirect target", "update_pc_target", 0x00004000u,
           dut.update_pc_target);

  dut.branch_update_pc = 0;
  dut.eval();
  check_eq("redirect branch false", "update_pc", 0, dut.update_pc);

  dut.branch_update_pc = 1;
  dut.decode_valid = 0;
  dut.eval();
  check_eq("redirect decode invalid", "update_pc", 0, dut.update_pc);
}

void test_taken_branch_during_stall() {
  Testbench tb;
  tb.reset();
  auto &dut = tb.dut();
  dut.memory_ready = 0;
  dut.decode_valid = 1;
  dut.decode_alu_op = ALU_ADD;
  dut.decode_op_src_b = OP_SRC_REG;
  dut.decode_rs1_data = 1;
  dut.decode_rs2_data = 2;
  dut.decode_rd_addr = 1;
  dut.decode_reg_we = 1;
  dut.alu_result = 3;
  tb.rising();
  const MemoryState held = tb.memory_state();
  check_eq("taken branch stall setup", "execute_ready", 0, dut.execute_ready);

  tb.clear_decode();
  dut.decode_valid = 1;
  dut.decode_branch_op = BRANCH_LT;
  dut.decode_rs1_data = 0xffffffffu;
  dut.decode_rs2_data = 1;
  dut.decode_pc = 0x00005000u;
  dut.decode_imm = 0xffffffe0u;
  dut.branch_update_pc = 1;
  dut.branch_update_pc_target = 0x00004fe0u;
  dut.eval();
  check_eq("taken branch stalled", "update_pc", 0, dut.update_pc);
  check_eq("taken branch stalled", "update_pc_target", 0x00004fe0u,
           dut.update_pc_target);
  tb.rising();
  check_memory_held("taken branch holds EX/MEM", held, dut);
  check_eq("taken branch remains stalled", "update_pc", 0, dut.update_pc);

  dut.memory_ready = 1;
  dut.eval();
  check_eq("taken branch accepted", "execute_ready", 1, dut.execute_ready);
  check_eq("taken branch accepted", "update_pc", 1, dut.update_pc);
  tb.rising();
  check_eq("taken branch captured", "memory_valid", 1, dut.memory_valid);
  check_eq("taken branch captured", "memory_reg_we", 0, dut.memory_reg_we);

  tb.clear_decode();
  dut.branch_update_pc = 1;
  dut.eval();
  check_eq("redirect does not repeat", "update_pc", 0, dut.update_pc);
}

// todo: fix env!!
[[maybe_unused]] void test_env() {}
} // namespace

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  test_reset_ready_stall_and_bubble();
  test_alu_paths();
  test_loads();
  test_stores();
  test_conditional_branches();
  test_jumps();
  test_redirect_formula();
  test_taken_branch_during_stall();
  test_lui_auipc();
  std::cout << checks << " execute checks passed\n";
  return 0;
}
