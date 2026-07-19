module memory_wrapper (
    input logic clk,
    input logic rst,
    input logic execute_valid,
    input logic [4:0] execute_lsu_op,
    input logic [31:0] execute_result,
    input logic [31:0] execute_store_data,
    input logic [4:0] execute_rd_addr,
    input logic execute_reg_we,
    output logic memory_ready,
    output logic writeback_valid,
    output logic [31:0] writeback_data,
    output logic [4:0] writeback_rd_addr,
    output logic writeback_reg_we,
    output logic [31:0] wb_adr_o,
    output logic [31:0] wb_dat_o,
    output logic [3:0] wb_sel_o,
    output logic wb_we_o,
    output logic wb_cyc_o,
    output logic wb_stb_o,
    input logic [31:0] wb_dat_i,
    input logic wb_ack_i
);

  memory u_memory (
      .clk,
      .rst,
      .execute_memory({
        execute_valid,
        execute_lsu_op,
        execute_result,
        execute_store_data,
        execute_rd_addr,
        execute_reg_we
      }),
      .memory_ready,
      .memory_writeback({writeback_valid, writeback_data, writeback_rd_addr, writeback_reg_we}),
      .wb_adr_o,
      .wb_dat_o,
      .wb_sel_o,
      .wb_we_o,
      .wb_cyc_o,
      .wb_stb_o,
      .wb_dat_i,
      .wb_ack_i
  );

endmodule
