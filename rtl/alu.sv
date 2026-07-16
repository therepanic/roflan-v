`include "decode_execute_pkg.sv"
import decode_execute_pkg::*;

module alu (
    input alu_op_e op,
    input logic [31:0] op_a,
    input logic [31:0] op_b,
    output logic [31:0] result
);

  always_comb begin
    result = 0;
    case (op)
      ALU_ADD:  result = op_a + op_b;
      ALU_SUB:  result = op_a - op_b;
      ALU_XOR:  result = op_a ^ op_b;
      ALU_OR:   result = op_a | op_b;
      ALU_AND:  result = op_a & op_b;
      ALU_SLL:  result = op_a << op_b[4:0];
      ALU_SRL:  result = op_a >> op_b[4:0];
      ALU_SRA:  result = (signed'(op_a)) >>> op_b[4:0];
      ALU_SLT:  result = (signed'(op_a) < signed'(op_b)) ? 1 : 0;
      ALU_SLTU: result = (op_a < op_b) ? 1 : 0;
    endcase
  end

endmodule
