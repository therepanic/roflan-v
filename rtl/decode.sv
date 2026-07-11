`include "decode_pkg.sv"
import decode_pkg::*;

module decode (
    input logic [31:0] instr,
    output decode_s decode_out_t,

    //register_file
    output logic [ 4:0] reg_rs1_addr,
    input  logic [31:0] reg_rs1_data,
    output logic [ 4:0] reg_rs2_addr,
    input  logic [31:0] reg_rs2_data
);
  logic [6:0] op;

  assign op = instr[6:0];

  always_comb begin
    decode_out_t = '0;
    reg_rs1_addr = '0;
    reg_rs2_addr = '0;
    case (op)
      7'b0110011: begin
        logic [4:0] rs1;
        logic [4:0] rs2;
        logic [6:0] funct7;
        logic [2:0] funct3;
        logic [4:0] rd;

        rs1 = instr[19:15];
        rs2 = instr[24:20];
        funct7 = instr[31:25];
        funct3 = instr[14:12];
        rd = instr[11:7];

        reg_rs1_addr = rs1;
        reg_rs2_addr = rs2;
        decode_out_t.op_src_b = OP_SRC_T_REG;
        decode_out_t.rs1_data = reg_rs1_data;
        decode_out_t.rs2_data = reg_rs2_data;
        decode_out_t.rd_addr = rd;

        if (funct3 == 3'h0 && funct7 == 7'h00) begin
          decode_out_t.op = ALU_ADD;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h0 && funct7 == 7'h20) begin
          decode_out_t.op = ALU_SUB;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h4 && funct7 == 7'h00) begin
          decode_out_t.op = ALU_XOR;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h6 && funct7 == 7'h00) begin
          decode_out_t.op = ALU_OR;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h7 && funct7 == 7'h00) begin
          decode_out_t.op = ALU_AND;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h1 && funct7 == 7'h00) begin
          decode_out_t.op = ALU_SLL;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h5 && funct7 == 7'h00) begin
          decode_out_t.op = ALU_SRL;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h5 && funct7 == 7'h20) begin
          decode_out_t.op = ALU_SRA;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h2 && funct7 == 7'h00) begin
          decode_out_t.op = ALU_SLT;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h3 && funct7 == 7'h00) begin
          decode_out_t.op = ALU_SLTU;
          decode_out_t.reg_we = 1'b1;
        end else begin
          decode_out_t.invalid = 1;
        end
      end
      7'b0010011: begin
        logic [ 4:0] rs1;
        logic [11:0] imm;
        logic [ 2:0] funct3;
        logic [ 4:0] rd;

        rs1 = instr[19:15];
        imm = instr[31:20];
        funct3 = instr[14:12];
        rd = instr[11:7];

        reg_rs1_addr = rs1;
        decode_out_t.op_src_b = OP_SRC_T_IMM;
        decode_out_t.rs1_data = reg_rs1_data;
        decode_out_t.rd_addr = rd;

        if (funct3 == 3'h0) begin
          decode_out_t.op = ALU_ADD;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else if (funct3 == 3'h4) begin
          decode_out_t.op = ALU_XOR;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else if (funct3 == 3'h6) begin
          decode_out_t.op = ALU_OR;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else if (funct3 == 3'h7) begin
          decode_out_t.op = ALU_AND;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else if (funct3 == 3'h1 && imm[11:5] == 7'h00) begin
          decode_out_t.op = ALU_SLL;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = imm[4:0];
        end else if (funct3 == 3'h5 && imm[11:5] == 7'h00) begin
          decode_out_t.op = ALU_SRL;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = imm[4:0];
        end else if (funct3 == 3'h5 && imm[11:5] == 7'h20) begin
          decode_out_t.op = ALU_SRA;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = imm[4:0];
        end else if (funct3 == 3'h2) begin
          decode_out_t.op = ALU_SLT;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else if (funct3 == 3'h3) begin
          decode_out_t.op = ALU_SLTU;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else begin
          decode_out_t.invalid = 1;
        end
      end
      default: begin
        decode_out_t.invalid = 1;
      end
    endcase
  end

endmodule
