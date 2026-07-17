`include "fetch_decode_pkg.sv"
import fetch_decode_pkg::*;

module fetch (
    input logic clk,
    input logic rst,
    output fetch_decode_s fetch_decode,

    input  logic decode_ready,
    output logic fetch_ready,

    //control_flowe
    input logic update_pc,
    input logic [31:0] update_pc_target,

    //wishbone interface
    output logic [31:0] wb_adr_o,
    output logic [31:0] wb_dat_o,
    output logic [3:0] wb_sel_o,
    output logic wb_we_o,
    output logic wb_cyc_o,
    output logic wb_stb_o,
    input logic [31:0] wb_dat_i,
    input logic wb_ack_i
);

  logic [31:0] pc = 32'h100;
  logic request_active;
  logic [31:0] request_pc;

  assign fetch_ready = !fetch_decode.valid || decode_ready;

  always_comb begin
    wb_adr_o = request_pc;
    wb_dat_o = '0;
    wb_sel_o = 4'b1111;
    wb_we_o  = 1'b0;
    wb_cyc_o = request_active;
    wb_stb_o = request_active;
  end

  always_ff @(posedge clk) begin
    if (rst) begin
      pc <= 32'h100;
      fetch_decode <= '0;
      request_active <= 1'b0;
      request_pc <= '0;
    end else if (update_pc) begin
      pc <= update_pc_target;
      fetch_decode.valid <= 1'b0;
      request_active <= 1'b0;
    end else begin
      if (fetch_decode.valid && decode_ready) begin
        fetch_decode.valid <= 1'b0;
      end
      if (request_active && wb_ack_i) begin
        request_active <= 1'b0;
        fetch_decode.valid <= 1'b1;
        fetch_decode.instr <= wb_dat_i;
        fetch_decode.pc <= request_pc;
        pc <= request_pc + 32'd4;
      end else if (!request_active && fetch_ready) begin
        request_active <= 1'b1;
        request_pc <= pc;
      end
    end
  end

endmodule
