`include "decode_execute_pkg.sv"
`include "execute_memory_pkg.sv"
import decode_execute_pkg::*;
import execute_memory_pkg::*;

module execute (
    input logic clk,
    input logic rst,

    input decode_execute_s decode_execute,
    input logic memory_ready,
    output logic execute_ready,
    output execute_memory_s execute_memory,

    output logic update_pc,
    output logic [31:0] update_pc_target,

    //alu
    output alu_op_e alu_op,
    output logic [31:0] alu_op_a,
    output logic [31:0] alu_op_b,
    input logic [31:0] alu_result,

    //control flow
    input logic branch_update_pc,
    input logic [31:0] branch_update_pc_target,
    output branch_op_e branch_op,
    output logic [31:0] branch_rs1_data,
    output logic [31:0] branch_rs2_data,
    output logic [31:0] branch_pc,
    output logic [31:0] branch_imm
);

  execute_memory_s req_execute_memory;

  assign execute_ready = !execute_memory.valid || memory_ready;

  assign update_pc = branch_update_pc && decode_execute.valid && execute_ready;

  assign update_pc_target = branch_update_pc_target;

  always_comb begin
    alu_op = ALU_NONE;
    alu_op_a = '0;
    alu_op_b = '0;
    branch_op = BRANCH_NONE;
    branch_rs1_data = '0;
    branch_rs2_data = '0;
    branch_pc = '0;
    branch_imm = '0;
    req_execute_memory = '0;

    if (decode_execute.valid) begin
      req_execute_memory.lsu_op = LSU_NONE;
      if (decode_execute.alu_op != ALU_NONE) begin
        alu_op   = decode_execute.alu_op;
        alu_op_a = decode_execute.rs1_data;
        if (decode_execute.op_src_b == OP_SRC_T_IMM) begin
          alu_op_b = decode_execute.imm;
        end else if (decode_execute.op_src_b == OP_SRC_T_REG) begin
          alu_op_b = decode_execute.rs2_data;
        end
        req_execute_memory.result  = alu_result;
        req_execute_memory.rd_addr = decode_execute.rd_addr;
        req_execute_memory.reg_we  = decode_execute.reg_we;
        req_execute_memory.valid   = 1'b1;
      end else if (decode_execute.branch_op != BRANCH_NONE) begin
        branch_op = decode_execute.branch_op;
        if (decode_execute.branch_op != JAL && decode_execute.branch_op != JAL_R) begin
          branch_rs1_data = decode_execute.rs1_data;
          branch_rs2_data = decode_execute.rs2_data;
        end else if (decode_execute.branch_op == JAL_R) begin
          branch_rs1_data = decode_execute.rs1_data;
        end
        branch_imm = decode_execute.imm;
        branch_pc  = decode_execute.pc;
        if (decode_execute.branch_op == JAL || decode_execute.branch_op == JAL_R) begin
          req_execute_memory.result  = decode_execute.pc + 4;
          req_execute_memory.reg_we  = decode_execute.reg_we;
          req_execute_memory.rd_addr = decode_execute.rd_addr;
        end
        req_execute_memory.valid = 1'b1;
      end else if (decode_execute.lsu_op != LSU_NONE) begin
        req_execute_memory.lsu_op = decode_execute.lsu_op;
        req_execute_memory.reg_we = decode_execute.reg_we;

        // alu
        alu_op = ALU_ADD;
        alu_op_a = decode_execute.rs1_data;
        alu_op_b = decode_execute.imm;

        req_execute_memory.result = alu_result;
        if (decode_execute.lsu_op == LSU_STORE_BYTE || decode_execute.lsu_op == LSU_STORE_HALF || decode_execute.lsu_op == LSU_STORE_WORD) begin
          req_execute_memory.store_data = decode_execute.rs2_data;
        end else begin
          req_execute_memory.rd_addr = decode_execute.rd_addr;
        end
        req_execute_memory.valid = 1'b1;
      end else if (decode_execute.pc_op != PC_NONE) begin
        req_execute_memory.rd_addr = decode_execute.rd_addr;
        req_execute_memory.reg_we  = decode_execute.reg_we;
        if (decode_execute.pc_op == LUI) begin
          req_execute_memory.result = decode_execute.imm << 12;
        end else if (decode_execute.pc_op == AUIPC) begin
          req_execute_memory.result = decode_execute.pc + (decode_execute.imm << 12);
        end
        req_execute_memory.valid = 1'b1;
      end else if (decode_execute.env_op != ENV_NONE) begin
        //TODO: there is no csr for now
      end
    end
  end


  always_ff @(posedge clk) begin
    if (rst) begin
      execute_memory <= 0;
    end else begin
      if (execute_ready) begin
        execute_memory <= req_execute_memory;
      end
    end
  end

endmodule
