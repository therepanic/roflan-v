module writeback_wrapper (
    input logic memory_valid,
    input logic [31:0] memory_wb_data,
    input logic [4:0] memory_rd_addr,
    input logic memory_reg_we,
    output logic reg_wr_en,
    output logic [4:0] reg_wr_addr,
    output logic [31:0] reg_wr_data
);

  writeback u_writeback (
      .memory_writeback({memory_valid, memory_wb_data, memory_rd_addr, memory_reg_we}),
      .reg_wr_en,
      .reg_wr_addr,
      .reg_wr_data
  );

endmodule
