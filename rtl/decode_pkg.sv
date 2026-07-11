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
    ALU_SLTU,
    ALU_ADD_I,
    ALU_SUB_I,
    ALU_XOR_I,
    ALU_OR_I,
    ALU_AND_I,
    ALU_SLL_I,
    ALU_SRL_I,
    ALU_SRA_I,
    ALU_SLT_I,
    ALU_SLTU_I
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
    logic [19:0] imm;
    logic reg_we;
    logic mem_re;
    logic mem_we;
    logic invalid;
  } decode_s;

endpackage : decode_pkg
