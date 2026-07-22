module roflan_v_wrapper #(
    parameter string SRAM_INIT_FILE = "tests/programs/roflan_v_integration.hex"
) (
    input logic clk,
    input logic rst,
    output logic reg_wr_en,
    output logic [  4:0] reg_wr_addr,
    output logic [ 31:0] reg_wr_data,
    output logic [ 31:0] dwb_adr_o,
    output logic [ 31:0] dwb_dat_o,
    output logic [  3:0] dwb_sel_o,
    output logic dwb_we_o,
    output logic dwb_cyc_o,
    output logic dwb_stb_o,
    output logic dwb_ack_i,
    output logic fetch_valid,
    output logic [ 31:0] fetch_pc,
    output logic [ 31:0] fetch_instr,
    output logic idex_valid,
    output logic [  4:0] idex_lsu_op,
    output logic [  4:0] idex_rd_addr,
    output logic idex_reg_we,
    output logic [158:0] idex_packet,
    output logic exmem_valid,
    output logic [  4:0] exmem_lsu_op,
    output logic [  4:0] exmem_rd_addr,
    output logic exmem_reg_we,
    output logic pending_valid,
    output logic [  4:0] pending_lsu_op,
    output logic [ 31:0] pending_result,
    output logic [ 31:0] pending_store_data,
    output logic [  4:0] pending_rd_addr,
    output logic pending_reg_we,
    output logic [ 75:0] pending_packet,
    output logic writeback_valid,
    output logic [  4:0] writeback_rd_addr,
    output logic writeback_reg_we,
    output logic fetch_ready,
    output logic decode_ready,
    output logic execute_ready,
    output logic memory_ready,
    output logic update_pc,
    output logic [ 31:0] update_pc_target
);

  roflan_v #(
      .MEM_BYTES(64 * 1024),
      .MEM_INIT_FILE(SRAM_INIT_FILE)
  ) dut (
      .clk,
      .rst
  );

  assign reg_wr_en = dut.reg_wr_en;
  assign reg_wr_addr = dut.reg_wr_addr;
  assign reg_wr_data = dut.reg_wr_data;
  assign dwb_adr_o = dut.dwb_adr_o;
  assign dwb_dat_o = dut.dwb_dat_o;
  assign dwb_sel_o = dut.dwb_sel_o;
  assign dwb_we_o = dut.dwb_we_o;
  assign dwb_cyc_o = dut.dwb_cyc_o;
  assign dwb_stb_o = dut.dwb_stb_o;
  assign dwb_ack_i = dut.dwb_ack_i;
  assign fetch_valid = dut.fetch_decode.valid;
  assign fetch_pc = dut.fetch_decode.pc;
  assign fetch_instr = dut.fetch_decode.instr;
  assign idex_valid = dut.decode_execute.valid;
  assign idex_lsu_op = dut.decode_execute.lsu_op;
  assign idex_rd_addr = dut.decode_execute.rd_addr;
  assign idex_reg_we = dut.decode_execute.reg_we;
  assign idex_packet = dut.decode_execute;
  assign exmem_valid = dut.execute_memory.valid;
  assign exmem_lsu_op = dut.execute_memory.lsu_op;
  assign exmem_rd_addr = dut.execute_memory.rd_addr;
  assign exmem_reg_we = dut.execute_memory.reg_we;
  assign pending_valid = dut.pending_execute_memory.valid;
  assign pending_lsu_op = dut.pending_execute_memory.lsu_op;
  assign pending_result = dut.pending_execute_memory.result;
  assign pending_store_data = dut.pending_execute_memory.store_data;
  assign pending_rd_addr = dut.pending_execute_memory.rd_addr;
  assign pending_reg_we = dut.pending_execute_memory.reg_we;
  assign pending_packet = dut.pending_execute_memory;
  assign writeback_valid = dut.memory_writeback.valid;
  assign writeback_rd_addr = dut.memory_writeback.rd_addr;
  assign writeback_reg_we = dut.memory_writeback.reg_we;
  assign fetch_ready = dut.fetch_ready;
  assign decode_ready = dut.decode_ready;
  assign execute_ready = dut.execute_ready;
  assign memory_ready = dut.memory_ready;
  assign update_pc = dut.update_pc;
  assign update_pc_target = dut.update_pc_target;

endmodule
