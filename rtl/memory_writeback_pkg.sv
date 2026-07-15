package memory_writeback_pkg;

  typedef struct packed {
    logic valid;
    logic [31:0] wb_data;
    logic [4:0] rd_addr;
    logic reg_we;
  } memory_writeback_s;

endpackage : memory_writeback_pkg
