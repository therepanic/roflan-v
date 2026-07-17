`include "decode_execute_pkg.sv"
`include "fetch_decode_pkg.sv"
import decode_execute_pkg::*;
import fetch_decode_pkg::*;

module decode (
    input logic clk,
    input logic rst,

    input logic flush,
    input fetch_decode_s fetch_decode,

    input  logic execute_ready,
    output logic decode_ready,

    output decode_execute_s decode_execute,

    //register_file
    output logic [ 4:0] reg_rs1_addr,
    input  logic [31:0] reg_rs1_data,
    output logic [ 4:0] reg_rs2_addr,
    input  logic [31:0] reg_rs2_data
);
  logic [6:0] op;
  logic [31:0] instr;

  decode_execute_s req_decode_execute;

  assign decode_ready = !decode_execute.valid || execute_ready;

  assign instr = fetch_decode.instr;

  assign op = instr[6:0];

  always_comb begin
    req_decode_execute = '0;
    reg_rs1_addr = '0;
    reg_rs2_addr = '0;
    if (fetch_decode.valid) begin
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
          req_decode_execute.op_src_b = OP_SRC_T_REG;
          req_decode_execute.rs1_data = reg_rs1_data;
          req_decode_execute.rs2_data = reg_rs2_data;
          req_decode_execute.rd_addr = rd;
          req_decode_execute.pc = fetch_decode.pc;

          if (funct3 == 3'h0 && funct7 == 7'h00) begin
            req_decode_execute.alu_op = ALU_ADD;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h0 && funct7 == 7'h20) begin
            req_decode_execute.alu_op = ALU_SUB;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h4 && funct7 == 7'h00) begin
            req_decode_execute.alu_op = ALU_XOR;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h6 && funct7 == 7'h00) begin
            req_decode_execute.alu_op = ALU_OR;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h7 && funct7 == 7'h00) begin
            req_decode_execute.alu_op = ALU_AND;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h1 && funct7 == 7'h00) begin
            req_decode_execute.alu_op = ALU_SLL;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h5 && funct7 == 7'h00) begin
            req_decode_execute.alu_op = ALU_SRL;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h5 && funct7 == 7'h20) begin
            req_decode_execute.alu_op = ALU_SRA;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h2 && funct7 == 7'h00) begin
            req_decode_execute.alu_op = ALU_SLT;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h3 && funct7 == 7'h00) begin
            req_decode_execute.alu_op = ALU_SLTU;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
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
          req_decode_execute.op_src_b = OP_SRC_T_IMM;
          req_decode_execute.rs1_data = reg_rs1_data;
          req_decode_execute.rd_addr = rd;
          req_decode_execute.pc = fetch_decode.pc;

          if (funct3 == 3'h0) begin
            req_decode_execute.alu_op = ALU_ADD;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.imm = signed'(imm);
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h4) begin
            req_decode_execute.alu_op = ALU_XOR;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.imm = signed'(imm);
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h6) begin
            req_decode_execute.alu_op = ALU_OR;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.imm = signed'(imm);
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h7) begin
            req_decode_execute.alu_op = ALU_AND;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.imm = signed'(imm);
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h1 && imm[11:5] == 7'h00) begin
            req_decode_execute.alu_op = ALU_SLL;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.imm = imm[4:0];
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h5 && imm[11:5] == 7'h00) begin
            req_decode_execute.alu_op = ALU_SRL;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.imm = imm[4:0];
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h5 && imm[11:5] == 7'h20) begin
            req_decode_execute.alu_op = ALU_SRA;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.imm = imm[4:0];
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h2) begin
            req_decode_execute.alu_op = ALU_SLT;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.imm = signed'(imm);
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h3) begin
            req_decode_execute.alu_op = ALU_SLTU;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.imm = signed'(imm);
            req_decode_execute.valid = 1;
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
          req_decode_execute.op_src_b = OP_SRC_T_IMM;
          req_decode_execute.rs1_data = reg_rs1_data;
          req_decode_execute.rd_addr = rd;
          req_decode_execute.imm = signed'(imm);
          req_decode_execute.pc = fetch_decode.pc;

          if (funct3 == 3'h0) begin
            req_decode_execute.lsu_op = LSU_LOAD_BYTE;
            req_decode_execute.mem_re = 1'b1;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h1) begin
            req_decode_execute.lsu_op = LSU_LOAD_HALF;
            req_decode_execute.mem_re = 1'b1;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h2) begin
            req_decode_execute.lsu_op = LSU_LOAD_WORD;
            req_decode_execute.mem_re = 1'b1;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h4) begin
            req_decode_execute.lsu_op = LSU_LOAD_BYTE_U;
            req_decode_execute.mem_re = 1'b1;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h5) begin
            req_decode_execute.lsu_op = LSU_LOAD_HALF_U;
            req_decode_execute.mem_re = 1'b1;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid  = 1;
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
          req_decode_execute.op_src_b = OP_SRC_T_IMM;
          req_decode_execute.rs1_data = reg_rs1_data;
          req_decode_execute.rs2_data = reg_rs2_data;
          req_decode_execute.imm = imm;
          req_decode_execute.pc = fetch_decode.pc;

          if (funct3 == 3'h0) begin
            req_decode_execute.lsu_op = LSU_STORE_BYTE;
            req_decode_execute.mem_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h1) begin
            req_decode_execute.lsu_op = LSU_STORE_HALF;
            req_decode_execute.mem_we = 1'b1;
            req_decode_execute.valid  = 1;
          end else if (funct3 == 3'h2) begin
            req_decode_execute.lsu_op = LSU_STORE_WORD;
            req_decode_execute.mem_we = 1'b1;
            req_decode_execute.valid  = 1;
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
          req_decode_execute.op_src_b = OP_SRC_T_REG;
          req_decode_execute.rs1_data = reg_rs1_data;
          req_decode_execute.rs2_data = reg_rs2_data;
          req_decode_execute.imm = imm;
          req_decode_execute.pc = fetch_decode.pc;

          if (funct3 == 3'h0) begin
            req_decode_execute.branch_op = BRANCH_EQ;
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h1) begin
            req_decode_execute.branch_op = BRANCH_NE;
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h4) begin
            req_decode_execute.branch_op = BRANCH_LT;
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h5) begin
            req_decode_execute.branch_op = BRANCH_GE;
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h6) begin
            req_decode_execute.branch_op = BRANCH_LTU;
            req_decode_execute.valid = 1;
          end else if (funct3 == 3'h7) begin
            req_decode_execute.branch_op = BRANCH_GEU;
            req_decode_execute.valid = 1;
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
          req_decode_execute.op_src_b = OP_SRC_T_IMM;
          req_decode_execute.rd_addr = rd;
          req_decode_execute.imm = imm;
          req_decode_execute.branch_op = JAL;
          req_decode_execute.reg_we = 1'b1;
          req_decode_execute.pc = fetch_decode.pc;
          req_decode_execute.valid = 1;
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
          req_decode_execute.op_src_b = OP_SRC_T_IMM;
          req_decode_execute.rs1_data = reg_rs1_data;
          req_decode_execute.rd_addr = rd;
          req_decode_execute.imm = signed'(imm);
          req_decode_execute.pc = fetch_decode.pc;

          if (funct3 == 3'h0) begin
            req_decode_execute.branch_op = JAL_R;
            req_decode_execute.reg_we = 1'b1;
            req_decode_execute.valid = 1;
          end
        end
        // Type U
        7'b0110111: begin
          logic [19:0] imm;
          logic [ 4:0] rd;

          imm = instr[31:12];
          rd = instr[11:7];
          req_decode_execute.op_src_b = OP_SRC_T_IMM;
          req_decode_execute.rd_addr = rd;
          req_decode_execute.imm = imm;
          req_decode_execute.pc_op = LUI;
          req_decode_execute.reg_we = 1'b1;
          req_decode_execute.pc = fetch_decode.pc;
          req_decode_execute.valid = 1;
        end
        7'b0010111: begin
          logic [19:0] imm;
          logic [ 4:0] rd;

          imm = instr[31:12];
          rd = instr[11:7];
          req_decode_execute.op_src_b = OP_SRC_T_IMM;
          req_decode_execute.rd_addr = rd;
          req_decode_execute.imm = imm;
          req_decode_execute.pc_op = AUIPC;
          req_decode_execute.reg_we = 1'b1;
          req_decode_execute.pc = fetch_decode.pc;
          req_decode_execute.valid = 1;
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
            req_decode_execute.env_op = ENV_CALL;
            req_decode_execute.valid = 1;
            req_decode_execute.pc = fetch_decode.pc;
          end else if (funct3 == 3'h0 && imm == 12'h1 && rs1 == 5'h0 && rd == 5'h0) begin
            req_decode_execute.env_op = ENV_BREAK;
            req_decode_execute.valid = 1;
            req_decode_execute.pc = fetch_decode.pc;
          end
        end
      endcase
    end
  end

  always_ff @(posedge clk) begin
    if (rst) begin
      decode_execute <= 0;
    end else begin
      if (flush) begin
        decode_execute.valid <= 1'b0;
      end else if (decode_ready) begin
        decode_execute <= req_decode_execute;
      end
    end
  end

endmodule
