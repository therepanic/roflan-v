`include "decode_pkg.sv"
import decode_pkg::*;

module branch (
    input branch_op_e op,
    input logic [31:0] rs1_data,
    input logic [31:0] rs2_data,
    input logic [31:0] pc,
    input logic [31:0] imm,
    output logic update_pc,
    output logic [31:0] update_pc_target
);

  always_comb begin
    update_pc = 0;
    update_pc_target = 0;
    case (op)
      BRANCH_EQ: begin
        if (rs1_data == rs2_data) begin
          update_pc = 1;
          update_pc_target = pc + imm;
        end
      end
      BRANCH_NE: begin
        if (rs1_data != rs2_data) begin
          update_pc = 1;
          update_pc_target = pc + imm;
        end
      end
      BRANCH_LT: begin
        if (signed'(rs1_data) < signed'(rs2_data)) begin
          update_pc = 1;
          update_pc_target = pc + imm;
        end
      end
      BRANCH_GE: begin
        if (signed'(rs1_data) >= signed'(rs2_data)) begin
          update_pc = 1;
          update_pc_target = pc + imm;
        end
      end
      BRANCH_LTU: begin
        if (rs1_data < rs2_data) begin
          update_pc = 1;
          update_pc_target = pc + imm;
        end
      end
      BRANCH_GEU: begin
        if (rs1_data >= rs2_data) begin
          update_pc = 1;
          update_pc_target = pc + imm;
        end
      end
      JAL: begin
        update_pc = 1;
        update_pc_target = pc + imm;
      end
      JAL_R: begin
        logic [31:0] sum;
        sum = rs1_data + imm;
        update_pc = 1;
        update_pc_target = {sum[31:1], 1'b0};
      end
    endcase
  end

endmodule
