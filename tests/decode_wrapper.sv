module decode_wrapper (
    input  logic         clk,
    input  logic         rst,
    input  logic         flush,
    input  logic [ 64:0] fetch_decode,
    input  logic         execute_ready,
    output logic         decode_ready,
    output logic [158:0] decode_execute,
    input  logic         execute_valid,
    input  logic [  4:0] execute_lsu_op,
    input  logic [ 31:0] execute_result,
    input  logic [ 31:0] execute_store_data,
    input  logic [  4:0] execute_rd_addr,
    input  logic         execute_reg_we,
    input  logic         pending_valid,
    input  logic [  4:0] pending_lsu_op,
    input  logic [ 31:0] pending_result,
    input  logic [ 31:0] pending_store_data,
    input  logic [  4:0] pending_rd_addr,
    input  logic         pending_reg_we,
    input  logic         writeback_valid,
    input  logic [ 31:0] writeback_data,
    input  logic [  4:0] writeback_rd_addr,
    input  logic         writeback_reg_we,
    output logic [  4:0] reg_rs1_addr,
    input  logic [ 31:0] reg_rs1_data,
    output logic [  4:0] reg_rs2_addr,
    input  logic [ 31:0] reg_rs2_data
);

  decode u_decode (
      .clk,
      .rst,
      .flush,
      .fetch_decode,
      .execute_ready,
      .decode_ready,
      .decode_execute,
      .execute_memory({
        execute_valid,
        execute_lsu_op,
        execute_result,
        execute_store_data,
        execute_rd_addr,
        execute_reg_we
      }),
      .pending_execute_memory({
        pending_valid,
        pending_lsu_op,
        pending_result,
        pending_store_data,
        pending_rd_addr,
        pending_reg_we
      }),
      .memory_writeback({writeback_valid, writeback_data, writeback_rd_addr, writeback_reg_we}),
      .reg_rs1_addr,
      .reg_rs1_data,
      .reg_rs2_addr,
      .reg_rs2_data
  );

endmodule
