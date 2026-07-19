module fetch_wrapper (
    input logic clk,
    input logic rst,
    output logic fetch_valid,
    output logic [31:0] fetch_instr,
    output logic [31:0] fetch_pc,
    input logic decode_ready,
    output logic fetch_ready,
    input logic update_pc,
    input logic [31:0] update_pc_target,
    output logic [31:0] wb_adr_o,
    output logic [31:0] wb_dat_o,
    output logic [3:0] wb_sel_o,
    output logic wb_we_o,
    output logic wb_cyc_o,
    output logic wb_stb_o,
    input logic [31:0] wb_dat_i,
    input logic wb_ack_i
);

  fetch u_fetch (
      .clk,
      .rst,
      .fetch_decode({fetch_valid, fetch_instr, fetch_pc}),
      .decode_ready,
      .fetch_ready,
      .update_pc,
      .update_pc_target,
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
