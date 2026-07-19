module sram #(
    // 5MB
    parameter int unsigned MEM_BYTES = 5 * 1024 * 1024,
    // todo add crt0
    parameter string INIT_FILE = ""
) (
    input logic clk,
    input logic rst,

    // fetch.sv
    input logic [31:0] wb_adr_o_1,
    input logic [31:0] wb_dat_o_1,
    input logic [3:0] wb_sel_o_1,
    input logic wb_we_o_1,
    input logic wb_cyc_o_1,
    input logic wb_stb_o_1,
    output logic [31:0] wb_dat_i_1,
    output logic wb_ack_i_1,

    // memory.sv
    input logic [31:0] wb_adr_o_2,
    input logic [31:0] wb_dat_o_2,
    input logic [3:0] wb_sel_o_2,
    input logic wb_we_o_2,
    input logic wb_cyc_o_2,
    input logic wb_stb_o_2,
    output logic [31:0] wb_dat_i_2,
    output logic wb_ack_i_2
);

  localparam int unsigned WORDS = MEM_BYTES / 4;

  logic [31:0] mem[0:WORDS - 1];

  logic request_pending_1;
  logic request_pending_2;

  logic [31:0] response_data_1;
  logic [31:0] response_data_2;

  logic [29:0] word_address_1;
  logic [29:0] word_address_2;

  assign word_address_1 = wb_adr_o_1[31:2];
  assign word_address_2 = wb_adr_o_2[31:2];

  assign wb_dat_i_1 = response_data_1;
  assign wb_dat_i_2 = response_data_2;

  assign wb_ack_i_1 = request_pending_1 && wb_cyc_o_1;
  assign wb_ack_i_2 = request_pending_2 && wb_cyc_o_2;

  integer i;
  initial begin
    for (i = 0; i < WORDS; i = i + 1) begin
      mem[i] = 32'b0;
    end

    if (INIT_FILE != "") begin
      $readmemh(INIT_FILE, mem);
    end
  end

  always_ff @(posedge clk) begin
    if (rst) begin
      request_pending_1 <= 1'b0;
      request_pending_2 <= 1'b0;
      response_data_1   <= 32'b0;
      response_data_2   <= 32'b0;
    end else begin
      if (request_pending_1) begin
        request_pending_1 <= 1'b0;
      end else if (wb_cyc_o_1 && wb_stb_o_1) begin
        request_pending_1 <= 1'b1;

        if (word_address_1 < WORDS) begin
          response_data_1 <= mem[word_address_1];

          if (wb_we_o_1) begin
            if (wb_sel_o_1[0]) mem[word_address_1][7:0] <= wb_dat_o_1[7:0];
            if (wb_sel_o_1[1]) mem[word_address_1][15:8] <= wb_dat_o_1[15:8];
            if (wb_sel_o_1[2]) mem[word_address_1][23:16] <= wb_dat_o_1[23:16];
            if (wb_sel_o_1[3]) mem[word_address_1][31:24] <= wb_dat_o_1[31:24];
          end
        end else begin
          response_data_1 <= 32'b0;
        end
      end

      if (request_pending_2) begin
        request_pending_2 <= 1'b0;
      end else if (wb_cyc_o_2 && wb_stb_o_2) begin
        request_pending_2 <= 1'b1;
        if (word_address_2 < WORDS) begin
          response_data_2 <= mem[word_address_2];

          if (wb_we_o_2) begin
            if (wb_sel_o_2[0]) mem[word_address_2][7:0] <= wb_dat_o_2[7:0];
            if (wb_sel_o_2[1]) mem[word_address_2][15:8] <= wb_dat_o_2[15:8];
            if (wb_sel_o_2[2]) mem[word_address_2][23:16] <= wb_dat_o_2[23:16];
            if (wb_sel_o_2[3]) mem[word_address_2][31:24] <= wb_dat_o_2[31:24];
          end
        end else begin
          response_data_2 <= 32'b0;
        end
      end
    end
  end

endmodule
