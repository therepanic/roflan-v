package decode_pkg;

  // make more cooler
  typedef enum logic [4:0] {
    ALU_ADD,
    ALU_SUB,
    ALU_XOR,
    ALU_OR,
    ALU_AND,
    ALU_SLL,
    ALU_SRL,
    ALU_SRA,
    ALU_SLT,
    ALU_SLTU
  } op_e;

  typedef enum logic [2:0] {
    OP_SRC_T_REG,
    OP_SRC_T_IMM
  } op_src_t_e;

  typedef struct packed {
    op_e op;
    op_src_t_e op_src_b;
    logic [31:0] rs1_data;
    logic [31:0] rs2_data;
    logic [4:0] rd_addr;
    logic signed [31:0] imm;
    logic reg_we;
    logic mem_re;
    logic mem_we;
    logic invalid;
  } decode_s;

endpackage : decode_pkg
