module register_file (
    input logic clk,
    input logic wr_en,
    input logic [4:0] wr_addr,
    input logic [63:0] wr_data,
    input logic [4:0] rs1_addr,
    output logic [63:0] rs1_data,
    input logic [4:0] rs2_addr,
    output logic [63:0] rs2_data
);

  logic [63:0] regs[0:31];

  always_ff @(posedge clk) begin
    if (wr_en && wr_addr != 0) begin
      regs[wr_addr] <= wr_data;
    end
  end

  always_comb begin
    rs1_data = regs[rs1_addr];
    rs2_data = regs[rs2_addr];
  end

endmodule
