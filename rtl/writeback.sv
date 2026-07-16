`include "decode_pkg.sv"
`include "execute_memory_pkg.sv"
`include "memory_writeback_pkg.sv"
import memory_writeback_pkg::*;

module writeback(
    input memory_writeback_s memory_writeback,

    //register_file
    output logic reg_wr_en,
    output logic [4:0] reg_wr_addr,
    output logic [31:0] reg_wr_data
);

    assign reg_wr_en = memory_writeback.valid && memory_writeback.reg_we;
    assign reg_wr_addr = memory_writeback.rd_addr;
    assign reg_wr_data = memory_writeback.wb_data;

endmodule
