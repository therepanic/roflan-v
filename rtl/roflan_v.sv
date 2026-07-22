`include "../peripherals/sram.sv"
`include "fetch_decode_pkg.sv"
`include "execute_memory_pkg.sv"
`include "memory_writeback_pkg.sv"

import fetch_decode_pkg::*;
import decode_execute_pkg::*;
import execute_memory_pkg::*;
import memory_writeback_pkg::*;

module roflan_v #(
    // 5 mb
    parameter int unsigned MEM_BYTES = 5 * 1024 * 1024,
    // todo
    parameter string MEM_INIT_FILE = ""
) (
    input logic clk,
    input logic rst
);

  logic [31:0] iwb_adr_o;
  logic [31:0] iwb_dat_o;
  logic [3:0] iwb_sel_o;
  logic iwb_we_o;
  logic iwb_cyc_o;
  logic iwb_stb_o;
  logic [31:0] iwb_dat_i;
  logic iwb_ack_i;

  logic [31:0] dwb_adr_o;
  logic [31:0] dwb_dat_o;
  logic [3:0] dwb_sel_o;
  logic dwb_we_o;
  logic dwb_cyc_o;
  logic dwb_stb_o;
  logic [31:0] dwb_dat_i;
  logic dwb_ack_i;

  fetch_decode_s fetch_decode;
  decode_execute_s decode_execute;
  execute_memory_s execute_memory;
  execute_memory_s pending_execute_memory;
  memory_writeback_s memory_writeback;

  logic fetch_ready;
  logic decode_ready;
  logic execute_ready;
  logic memory_ready;

  logic update_pc;
  logic [31:0] update_pc_target;

  logic [4:0] reg_rs1_addr;
  logic [31:0] reg_rs1_data;
  logic [4:0] reg_rs2_addr;
  logic [31:0] reg_rs2_data;
  logic reg_wr_en;
  logic [4:0] reg_wr_addr;
  logic [31:0] reg_wr_data;

  alu_op_e alu_op;
  logic [31:0] alu_op_a;
  logic [31:0] alu_op_b;
  logic [31:0] alu_result;

  logic branch_update_pc;
  logic [31:0] branch_update_pc_target;
  branch_op_e branch_op;
  logic [31:0] branch_rs1_data;
  logic [31:0] branch_rs2_data;
  logic [31:0] branch_pc;
  logic [31:0] branch_imm;

  // 1) INSTRUCTION FETCH STAGE
  fetch u_fetch (
      .clk(clk),
      .rst(rst),
      .fetch_decode(fetch_decode),
      .decode_ready(decode_ready),
      .fetch_ready(fetch_ready),
      .update_pc(update_pc),
      .update_pc_target(update_pc_target),
      .wb_adr_o(iwb_adr_o),
      .wb_dat_o(iwb_dat_o),
      .wb_sel_o(iwb_sel_o),
      .wb_we_o(iwb_we_o),
      .wb_cyc_o(iwb_cyc_o),
      .wb_stb_o(iwb_stb_o),
      .wb_dat_i(iwb_dat_i),
      .wb_ack_i(iwb_ack_i)
  );

  // 2) INSTRUCTION DECODE STAGE
  decode u_decode (
      .clk(clk),
      .rst(rst),
      .flush(update_pc),
      .fetch_decode(fetch_decode),
      .execute_ready(execute_ready),
      .decode_ready(decode_ready),
      .decode_execute(decode_execute),
      .execute_memory(execute_memory),
      .pending_execute_memory(pending_execute_memory),
      .memory_writeback(memory_writeback),
      .reg_rs1_addr(reg_rs1_addr),
      .reg_rs1_data(reg_rs1_data),
      .reg_rs2_addr(reg_rs2_addr),
      .reg_rs2_data(reg_rs2_data)
  );

  // 3) INSTRUCTION EXECUTION STAGE
  execute u_execute (
      .clk(clk),
      .rst(rst),
      .decode_execute(decode_execute),
      .memory_ready(memory_ready),
      .execute_ready(execute_ready),
      .execute_memory(execute_memory),
      .update_pc(update_pc),
      .update_pc_target(update_pc_target),
      .alu_op(alu_op),
      .alu_op_a(alu_op_a),
      .alu_op_b(alu_op_b),
      .alu_result(alu_result),
      .branch_update_pc(branch_update_pc),
      .branch_update_pc_target(branch_update_pc_target),
      .branch_op(branch_op),
      .branch_rs1_data(branch_rs1_data),
      .branch_rs2_data(branch_rs2_data),
      .branch_pc(branch_pc),
      .branch_imm(branch_imm)
  );

  // 4) INSTRUCTION MEMORY STAGE
  memory u_memory (
      .clk(clk),
      .rst(rst),
      .execute_memory(execute_memory),
      .memory_ready(memory_ready),
      .memory_writeback(memory_writeback),
      .pending_execute_memory(pending_execute_memory),
      .wb_adr_o(dwb_adr_o),
      .wb_dat_o(dwb_dat_o),
      .wb_sel_o(dwb_sel_o),
      .wb_we_o(dwb_we_o),
      .wb_cyc_o(dwb_cyc_o),
      .wb_stb_o(dwb_stb_o),
      .wb_dat_i(dwb_dat_i),
      .wb_ack_i(dwb_ack_i)
  );

  // 5) INSTRUCTION WRITEBACK STAGE
  writeback u_writeback (
      .memory_writeback(memory_writeback),
      .reg_wr_en(reg_wr_en),
      .reg_wr_addr(reg_wr_addr),
      .reg_wr_data(reg_wr_data)
  );

  register_file u_register_file (
      .clk(clk),
      .wr_en(reg_wr_en),
      .wr_addr(reg_wr_addr),
      .wr_data(reg_wr_data),
      .rs1_addr(reg_rs1_addr),
      .rs1_data(reg_rs1_data),
      .rs2_addr(reg_rs2_addr),
      .rs2_data(reg_rs2_data)
  );

  alu u_alu (
      .op(alu_op),
      .op_a(alu_op_a),
      .op_b(alu_op_b),
      .result(alu_result)
  );

  control_flow u_control_flow (
      .op(branch_op),
      .rs1_data(branch_rs1_data),
      .rs2_data(branch_rs2_data),
      .pc(branch_pc),
      .imm(branch_imm),
      .update_pc(branch_update_pc),
      .update_pc_target(branch_update_pc_target)
  );

  // todo: we want to use sdram!
  sram #(
      .MEM_BYTES(MEM_BYTES),
      .INIT_FILE(MEM_INIT_FILE)
  ) u_sram (
      .clk(clk),
      .rst(rst),
      .wb_adr_o_1(iwb_adr_o),
      .wb_dat_o_1(iwb_dat_o),
      .wb_sel_o_1(iwb_sel_o),
      .wb_we_o_1(iwb_we_o),
      .wb_cyc_o_1(iwb_cyc_o),
      .wb_stb_o_1(iwb_stb_o),
      .wb_dat_i_1(iwb_dat_i),
      .wb_ack_i_1(iwb_ack_i),
      .wb_adr_o_2(dwb_adr_o),
      .wb_dat_o_2(dwb_dat_o),
      .wb_sel_o_2(dwb_sel_o),
      .wb_we_o_2(dwb_we_o),
      .wb_cyc_o_2(dwb_cyc_o),
      .wb_stb_o_2(dwb_stb_o),
      .wb_dat_i_2(dwb_dat_i),
      .wb_ack_i_2(dwb_ack_i)
  );

endmodule
