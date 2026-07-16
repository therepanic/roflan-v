`include "decode_execute_pkg.sv"
`include "execute_memory_pkg.sv"
`include "memory_writeback_pkg.sv"
import decode_execute_pkg::*;
import execute_memory_pkg::*;
import memory_writeback_pkg::*;

module memory (
    input logic clk,
    input logic rst,

    input execute_memory_s execute_memory,

    output logic memory_ready,

    output memory_writeback_s memory_writeback,

    //wishbone interface
    output logic [31:0] wb_adr_o,
    output logic [31:0] wb_dat_o,
    output logic [3:0] wb_sel_o,
    output logic wb_we_o,
    output logic wb_cyc_o,
    output logic wb_stb_o,
    input logic [31:0] wb_dat_i,
    input logic wb_ack_i
);

  execute_memory_s request_q;

  always_comb begin
    memory_ready = !request_q.valid;

    wb_adr_o = 32'b0;
    wb_dat_o = 32'b0;
    wb_sel_o = 4'b0000;
    wb_we_o = 1'b0;
    wb_cyc_o = 1'b0;
    wb_stb_o = 1'b0;

    if (request_q.valid) begin
      wb_adr_o = request_q.result;
      wb_cyc_o = 1'b1;
      wb_stb_o = 1'b1;

      case (request_q.op)
        LSU_LOAD_BYTE, LSU_LOAD_BYTE_U: begin
          wb_we_o = 1'b0;

          case (request_q.result[1:0])
            2'b00: wb_sel_o = 4'b0001;
            2'b01: wb_sel_o = 4'b0010;
            2'b10: wb_sel_o = 4'b0100;
            2'b11: wb_sel_o = 4'b1000;
          endcase
        end

        LSU_LOAD_HALF, LSU_LOAD_HALF_U: begin
          wb_we_o = 1'b0;

          case (request_q.result[1])
            1'b0: wb_sel_o = 4'b0011;
            1'b1: wb_sel_o = 4'b1100;
          endcase
        end

        LSU_LOAD_WORD: begin
          wb_we_o  = 1'b0;
          wb_sel_o = 4'b1111;
        end

        LSU_STORE_BYTE: begin
          wb_we_o = 1'b1;

          case (request_q.result[1:0])
            2'b00: begin
              wb_sel_o = 4'b0001;
              wb_dat_o = {24'b0, request_q.store_data[7:0]};
            end

            2'b01: begin
              wb_sel_o = 4'b0010;
              wb_dat_o = {16'b0, request_q.store_data[7:0], 8'b0};
            end

            2'b10: begin
              wb_sel_o = 4'b0100;
              wb_dat_o = {8'b0, request_q.store_data[7:0], 16'b0};
            end

            2'b11: begin
              wb_sel_o = 4'b1000;
              wb_dat_o = {request_q.store_data[7:0], 24'b0};
            end
          endcase
        end

        LSU_STORE_HALF: begin
          wb_we_o = 1'b1;

          case (request_q.result[1])
            1'b0: begin
              wb_sel_o = 4'b0011;
              wb_dat_o = {16'b0, request_q.store_data[15:0]};
            end

            1'b1: begin
              wb_sel_o = 4'b1100;
              wb_dat_o = {request_q.store_data[15:0], 16'b0};
            end
          endcase
        end

        LSU_STORE_WORD: begin
          wb_we_o  = 1'b1;
          wb_sel_o = 4'b1111;
          wb_dat_o = request_q.store_data;
        end

        default: begin
          wb_cyc_o = 1'b0;
          wb_stb_o = 1'b0;
        end
      endcase
    end
  end

  always_ff @(posedge clk) begin
    if (rst) begin
      request_q <= '0;
      memory_writeback <= '0;
    end else begin
      if (memory_writeback.valid) begin
        memory_writeback.valid <= 1'b0;
      end
      if (request_q.valid && wb_ack_i) begin
        request_q.valid <= 1'b0;

        memory_writeback.valid <= 1'b1;
        memory_writeback.rd_addr <= request_q.rd_addr;
        memory_writeback.reg_we <= request_q.reg_we;
        memory_writeback.wb_data <= 32'b0;

        case (request_q.op)
          LSU_LOAD_BYTE: begin
            case (request_q.result[1:0])
              2'b00: memory_writeback.wb_data <= {{24{wb_dat_i[7]}}, wb_dat_i[7:0]};
              2'b01: memory_writeback.wb_data <= {{24{wb_dat_i[15]}}, wb_dat_i[15:8]};
              2'b10: memory_writeback.wb_data <= {{24{wb_dat_i[23]}}, wb_dat_i[23:16]};
              2'b11: memory_writeback.wb_data <= {{24{wb_dat_i[31]}}, wb_dat_i[31:24]};
            endcase
          end

          LSU_LOAD_BYTE_U: begin
            case (request_q.result[1:0])
              2'b00: memory_writeback.wb_data <= {24'b0, wb_dat_i[7:0]};
              2'b01: memory_writeback.wb_data <= {24'b0, wb_dat_i[15:8]};
              2'b10: memory_writeback.wb_data <= {24'b0, wb_dat_i[23:16]};
              2'b11: memory_writeback.wb_data <= {24'b0, wb_dat_i[31:24]};
            endcase
          end

          LSU_LOAD_HALF: begin
            case (request_q.result[1])
              1'b0: memory_writeback.wb_data <= {{16{wb_dat_i[15]}}, wb_dat_i[15:0]};
              1'b1: memory_writeback.wb_data <= {{16{wb_dat_i[31]}}, wb_dat_i[31:16]};
            endcase
          end

          LSU_LOAD_HALF_U: begin
            case (request_q.result[1])
              1'b0: memory_writeback.wb_data <= {16'b0, wb_dat_i[15:0]};
              1'b1: memory_writeback.wb_data <= {16'b0, wb_dat_i[31:16]};
            endcase
          end
          LSU_LOAD_WORD: begin
            memory_writeback.wb_data <= wb_dat_i;
          end
          LSU_STORE_BYTE, LSU_STORE_HALF, LSU_STORE_WORD: begin
            memory_writeback.wb_data <= 32'b0;
          end
          default: begin
            memory_writeback.wb_data <= 32'b0;
          end
        endcase
      end else if (execute_memory.valid && memory_ready) begin
        case (execute_memory.op)
          LSU_LOAD_BYTE,
          LSU_LOAD_HALF,
          LSU_LOAD_WORD,
          LSU_LOAD_BYTE_U,
          LSU_LOAD_HALF_U,
          LSU_STORE_BYTE,
          LSU_STORE_HALF,
          LSU_STORE_WORD: begin
            request_q <= execute_memory;
          end
          default: begin
            memory_writeback.valid   <= 1'b1;
            memory_writeback.wb_data <= execute_memory.result;
            memory_writeback.rd_addr <= execute_memory.rd_addr;
            memory_writeback.reg_we  <= execute_memory.reg_we;
          end
        endcase
      end
    end
  end

endmodule
