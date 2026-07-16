`include "decode_execute_pkg.sv"
import decode_execute_pkg::*;

package execute_memory_pkg;

  typedef struct packed {
    logic valid;
    op_e op;
    logic [31:0] result;
    logic [31:0] store_data;
    logic [4:0] rd_addr;
    logic reg_we;
  } execute_memory_s;

endpackage : execute_memory_pkg
