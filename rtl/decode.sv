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
      // Type R
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
          decode_out_t.alu_op = ALU_ADD;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h0 && funct7 == 7'h20) begin
          decode_out_t.alu_op = ALU_SUB;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h4 && funct7 == 7'h00) begin
          decode_out_t.alu_op = ALU_XOR;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h6 && funct7 == 7'h00) begin
          decode_out_t.alu_op = ALU_OR;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h7 && funct7 == 7'h00) begin
          decode_out_t.alu_op = ALU_AND;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h1 && funct7 == 7'h00) begin
          decode_out_t.alu_op = ALU_SLL;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h5 && funct7 == 7'h00) begin
          decode_out_t.alu_op = ALU_SRL;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h5 && funct7 == 7'h20) begin
          decode_out_t.alu_op = ALU_SRA;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h2 && funct7 == 7'h00) begin
          decode_out_t.alu_op = ALU_SLT;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h3 && funct7 == 7'h00) begin
          decode_out_t.alu_op = ALU_SLTU;
          decode_out_t.reg_we = 1'b1;
        end else begin
          decode_out_t.invalid = 1;
        end
      end
      // Type I
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
          decode_out_t.alu_op = ALU_ADD;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else if (funct3 == 3'h4) begin
          decode_out_t.alu_op = ALU_XOR;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else if (funct3 == 3'h6) begin
          decode_out_t.alu_op = ALU_OR;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else if (funct3 == 3'h7) begin
          decode_out_t.alu_op = ALU_AND;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else if (funct3 == 3'h1 && imm[11:5] == 7'h00) begin
          decode_out_t.alu_op = ALU_SLL;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = imm[4:0];
        end else if (funct3 == 3'h5 && imm[11:5] == 7'h00) begin
          decode_out_t.alu_op = ALU_SRL;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = imm[4:0];
        end else if (funct3 == 3'h5 && imm[11:5] == 7'h20) begin
          decode_out_t.alu_op = ALU_SRA;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = imm[4:0];
        end else if (funct3 == 3'h2) begin
          decode_out_t.alu_op = ALU_SLT;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else if (funct3 == 3'h3) begin
          decode_out_t.alu_op = ALU_SLTU;
          decode_out_t.reg_we = 1'b1;
          decode_out_t.imm = signed'(imm);
        end else begin
          decode_out_t.invalid = 1;
        end
      end
      // Type I
      7'b0000011: begin
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
        decode_out_t.imm = signed'(imm);
        if (funct3 == 3'h0) begin
          decode_out_t.op = LSU_LOAD_BYTE;
          decode_out_t.mem_re = 1'b1;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h1) begin
          decode_out_t.op = LSU_LOAD_HALF;
          decode_out_t.mem_re = 1'b1;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h2) begin
          decode_out_t.op = LSU_LOAD_WORD;
          decode_out_t.mem_re = 1'b1;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h4) begin
          decode_out_t.op = LSU_LOAD_BYTE_U;
          decode_out_t.mem_re = 1'b1;
          decode_out_t.reg_we = 1'b1;
        end else if (funct3 == 3'h5) begin
          decode_out_t.op = LSU_LOAD_HALF_U;
          decode_out_t.mem_re = 1'b1;
          decode_out_t.reg_we = 1'b1;
        end else begin
          decode_out_t.invalid = 1;
        end
      end
      // Type S
      7'b0100011: begin
        logic [ 2:0] funct3;
        logic [ 4:0] rs1;
        logic [ 4:0] rs2;

        logic [31:0] imm;
        imm[4:0] = instr[11:7];
        imm[11:5] = instr[31:25];
        imm[31:12] = {20{instr[31]}};

        funct3 = instr[14:12];
        rs1 = instr[19:15];
        rs2 = instr[24:20];

        reg_rs1_addr = rs1;
        reg_rs2_addr = rs2;
        decode_out_t.op_src_b = OP_SRC_T_IMM;
        decode_out_t.rs1_data = reg_rs1_data;
        decode_out_t.rs2_data = reg_rs2_data;
        decode_out_t.imm = imm;

        if (funct3 == 3'h0) begin
          decode_out_t.op = LSU_STORE_BYTE;
          decode_out_t.mem_we = 1'b1;
        end else if (funct3 == 3'h1) begin
          decode_out_t.op = LSU_STORE_HALF;
          decode_out_t.mem_we = 1'b1;
        end else if (funct3 == 3'h2) begin
          decode_out_t.op = LSU_STORE_WORD;
          decode_out_t.mem_we = 1'b1;
        end else begin
          decode_out_t.invalid = 1;
        end
      end
      // Type B
      7'b1100011: begin
        logic [ 2:0] funct3;
        logic [ 4:0] rs1;
        logic [ 4:0] rs2;

        logic [31:0] imm;
        imm[0] = 0;
        imm[4:1] = instr[11:8];
        imm[10:5] = instr[30:25];
        imm[11] = instr[7];
        imm[12] = instr[31];
        imm[31:13] = {19{instr[31]}};

        funct3 = instr[14:12];
        rs1 = instr[19:15];
        rs2 = instr[24:20];

        reg_rs1_addr = rs1;
        reg_rs2_addr = rs2;
        decode_out_t.op_src_b = OP_SRC_T_REG;
        decode_out_t.rs1_data = reg_rs1_data;
        decode_out_t.rs2_data = reg_rs2_data;
        decode_out_t.imm = imm;

        if (funct3 == 3'h0) begin
          decode_out_t.op = BRANCH_EQ;
        end else if (funct3 == 3'h1) begin
          decode_out_t.op = BRANCH_NE;
        end else if (funct3 == 3'h4) begin
          decode_out_t.op = BRANCH_LT;
        end else if (funct3 == 3'h5) begin
          decode_out_t.op = BRANCH_GE;
        end else if (funct3 == 3'h6) begin
          decode_out_t.op = BRANCH_LTU;
        end else if (funct3 == 3'h7) begin
          decode_out_t.op = BRANCH_GEU;
        end else begin
          decode_out_t.invalid = 1;
        end
      end
      // Type J
      7'b1101111: begin
        logic [31:0] imm;
        logic [ 4:0] rd;

        rd = instr[11:7];
        imm[0] = 0;
        imm[10:1] = instr[30:21];
        imm[11] = instr[20];
        imm[19:12] = instr[19:12];
        imm[20] = instr[31];
        imm[31:21] = {11{instr[31]}};
        decode_out_t.op_src_b = OP_SRC_T_IMM;
        decode_out_t.rd_addr = rd;
        decode_out_t.imm = imm;
        decode_out_t.op = JAL;
        decode_out_t.reg_we = 1'b1;
      end
      // Type I, but jump
      7'b1100111: begin
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
        decode_out_t.imm = signed'(imm);
        if (funct3 == 3'h0) begin
          decode_out_t.op = JAL_R;
          decode_out_t.reg_we = 1'b1;
        end else begin
          decode_out_t.invalid = 1;
        end
      end
      // Type U
      7'b0110111: begin
        logic [19:0] imm;
        logic [ 4:0] rd;

        imm = instr[31:12];
        rd = instr[11:7];
        decode_out_t.op_src_b = OP_SRC_T_IMM;
        decode_out_t.rd_addr = rd;
        decode_out_t.imm = imm;
        decode_out_t.op = LUI;
        decode_out_t.reg_we = 1'b1;
      end
      7'b0010111: begin
        logic [19:0] imm;
        logic [ 4:0] rd;

        imm = instr[31:12];
        rd = instr[11:7];
        decode_out_t.op_src_b = OP_SRC_T_IMM;
        decode_out_t.rd_addr = rd;
        decode_out_t.imm = imm;
        decode_out_t.op = AUIP;
        decode_out_t.reg_we = 1'b1;
      end
      // Type I but for ENV
      7'b1110011: begin
        logic [11:0] imm;
        logic [ 2:0] funct3;
        logic [ 4:0] rs1;
        logic [ 4:0] rd;

        imm = instr[31:20];
        funct3 = instr[14:12];
        rd = instr[11:7];
        rs1 = instr[19:15];

        if (funct3 == 3'h0 && imm == 12'h0 && rs1 == 5'h0 && rd == 5'h0) begin
          decode_out_t.op = ENV_CALL;
        end else if (funct3 == 3'h0 && imm == 12'h1 && rs1 == 5'h0 && rd == 5'h0) begin
          decode_out_t.op = ENV_BREAK;
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
