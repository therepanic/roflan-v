package fetch_decode_pkg;

  typedef struct packed {
    logic valid;
    logic [31:0] instr;
    logic [31:0] pc;
  } fetch_decode_s;

endpackage : fetch_decode_pkg
