package decode_pkg;

  // make more cooler
  typedef enum logic [4:0] {
    LSU_LOAD_BYTE,
    LSU_LOAD_HALF,
    LSU_LOAD_WORD,
    LSU_LOAD_BYTE_U,
    LSU_LOAD_HALF_U,
    LSU_STORE_BYTE,
    LSU_STORE_HALF,
    LSU_STORE_WORD,

    LUI,
    AUIP,

    ENV_CALL,
    ENV_BREAK
  } op_e;

  typedef enum logic [2:0] {
    BRANCH_EQ,
    BRANCH_NE,
    BRANCH_LT,
    BRANCH_GE,
    BRANCH_LTU,
    BRANCH_GEU,
    JAL,
    JAL_R
  } branch_op_e;

  typedef enum logic [3:0] {
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
  } alu_op_e;

  typedef enum logic [2:0] {
    OP_SRC_T_REG,
    OP_SRC_T_IMM
  } op_src_t_e;

  typedef struct packed {
    op_e op;
    alu_op_e alu_op;
    branch_op_e branch_op;
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
