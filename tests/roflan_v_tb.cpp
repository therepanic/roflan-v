#include "Vroflan_v_wrapper.h"
#include "verilated.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct RegWrite {
  uint64_t cycle;
  uint8_t rd;
  uint32_t data;
};

struct Store {
  uint64_t cycle;
  uint32_t address;
  uint32_t data;
  uint8_t sel;
};

struct Snapshot {
  bool rst;
  bool fetch_valid;
  uint32_t fetch_pc;
  uint32_t fetch_instr;
  bool idex_valid;
  bool exmem_valid;
  bool pending_valid;
  bool writeback_valid;
  bool fetch_ready;
  bool decode_ready;
  bool execute_ready;
  bool memory_ready;
  bool update_pc;
  uint32_t update_pc_target;
  bool dwb_cyc;
  bool dwb_stb;
  bool dwb_ack;
  bool dwb_we;
  uint32_t dwb_address;
  uint32_t dwb_data;
  uint8_t dwb_sel;
  uint8_t idex_rd;
  bool idex_reg_we;
  uint8_t exmem_rd;
  bool exmem_reg_we;
  uint8_t pending_rd;
  bool pending_reg_we;
  uint8_t writeback_rd;
  bool writeback_reg_we;
  std::array<uint32_t, 5> idex_packet;
  std::array<uint32_t, 3> pending_packet;
};

class IntegrationTest {
public:
  explicit IntegrationTest(Vroflan_v_wrapper &dut) : dut_(dut) {}

  int run() {
    dut_.clk = 0;
    dut_.rst = 1;
    dut_.eval();
    for (int i = 0; i < 3; ++i)
      step();
    dut_.rst = 0;
    dut_.eval();
    while (!finished_ && cycle_ < 5000)
      step();
    if (!finished_)
      fail("timeout", "success signature", 0x600d600d, 0);
    verify_results();
    std::cout << "All roflan-v integration tests passed\n";
    std::cout << "cycles: " << cycle_ << "\n";
    std::cout << "register writes: " << writes_.size() << "\n";
    std::cout << "stores: " << stores_.size() << "\n";
    std::cout << "per-cycle checks: " << checks_ << "\n";
    std::cout << "success signature received\n";
    return 0;
  }

private:
  Vroflan_v_wrapper &dut_;
  uint64_t cycle_ = 0;
  uint64_t checks_ = 0;
  bool finished_ = false;
  bool saw_load_pending_window_ = false;
  bool saw_raw_rs1_hazard_ = false;
  bool saw_raw_rs2_hazard_ = false;
  bool saw_load_use_hazard_ = false;
  bool saw_load_use_release_ = false;
  bool saw_redirect_ = false;
  uint32_t auipc_pc_ = 0;
  uint32_t jal_pc_ = 0;
  uint32_t jalr_pc_ = 0;
  unsigned load_consumer_captures_ = 0;
  unsigned raw_rs1_consumer_captures_ = 0;
  unsigned raw_rs2_consumer_captures_ = 0;
  std::array<uint32_t, 32> registers_{};
  std::vector<RegWrite> writes_;
  std::vector<Store> stores_;
  std::map<uint32_t, unsigned> accepted_pc_;

  static std::string hex(uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
    return out.str();
  }

  Snapshot snapshot() const {
    Snapshot s{};
    s.rst = dut_.rst;
    s.fetch_valid = dut_.fetch_valid;
    s.fetch_pc = dut_.fetch_pc;
    s.fetch_instr = dut_.fetch_instr;
    s.idex_valid = dut_.idex_valid;
    s.exmem_valid = dut_.exmem_valid;
    s.pending_valid = dut_.pending_valid;
    s.writeback_valid = dut_.writeback_valid;
    s.fetch_ready = dut_.fetch_ready;
    s.decode_ready = dut_.decode_ready;
    s.execute_ready = dut_.execute_ready;
    s.memory_ready = dut_.memory_ready;
    s.update_pc = dut_.update_pc;
    s.update_pc_target = dut_.update_pc_target;
    s.dwb_cyc = dut_.dwb_cyc_o;
    s.dwb_stb = dut_.dwb_stb_o;
    s.dwb_ack = dut_.dwb_ack_i;
    s.dwb_we = dut_.dwb_we_o;
    s.dwb_address = dut_.dwb_adr_o;
    s.dwb_data = dut_.dwb_dat_o;
    s.dwb_sel = dut_.dwb_sel_o;
    s.idex_rd = dut_.idex_rd_addr;
    s.idex_reg_we = dut_.idex_reg_we;
    s.exmem_rd = dut_.exmem_rd_addr;
    s.exmem_reg_we = dut_.exmem_reg_we;
    s.pending_rd = dut_.pending_rd_addr;
    s.pending_reg_we = dut_.pending_reg_we;
    s.writeback_rd = dut_.writeback_rd_addr;
    s.writeback_reg_we = dut_.writeback_reg_we;
    for (size_t i = 0; i < s.idex_packet.size(); ++i)
      s.idex_packet[i] = dut_.idex_packet[i];
    for (size_t i = 0; i < s.pending_packet.size(); ++i)
      s.pending_packet[i] = dut_.pending_packet[i];
    return s;
  }

  [[noreturn]] void fail(const std::string &test, const std::string &field,
                         uint32_t expected, uint32_t actual) const {
    std::cerr << "FAIL: " << test << "\n";
    std::cerr << "cycle: " << cycle_ << "\n";
    std::cerr << "IF/ID PC: " << hex(dut_.fetch_pc)
              << " instruction: " << hex(dut_.fetch_instr)
              << " valid: " << unsigned(dut_.fetch_valid) << "\n";
    std::cerr << "decode_ready: " << unsigned(dut_.decode_ready)
              << " execute_ready: " << unsigned(dut_.execute_ready)
              << " memory_ready: " << unsigned(dut_.memory_ready) << "\n";
    std::cerr << "pipeline valid: " << unsigned(dut_.fetch_valid) << "/"
              << unsigned(dut_.idex_valid) << "/" << unsigned(dut_.exmem_valid)
              << "/" << unsigned(dut_.pending_valid) << "/"
              << unsigned(dut_.writeback_valid) << "\n";
    std::cerr << "field: " << field << " expected: " << hex(expected)
              << " actual: " << hex(actual) << "\n";
    std::cerr << "last register writes:\n";
    for (size_t i = writes_.size() > 8 ? writes_.size() - 8 : 0;
         i < writes_.size(); ++i)
      std::cerr << "  cycle " << writes_[i].cycle << " x"
                << unsigned(writes_[i].rd) << " = " << hex(writes_[i].data)
                << "\n";
    std::cerr << "last stores:\n";
    for (size_t i = stores_.size() > 8 ? stores_.size() - 8 : 0;
         i < stores_.size(); ++i)
      std::cerr << "  cycle " << stores_[i].cycle
                << " adr=" << hex(stores_[i].address)
                << " data=" << hex(stores_[i].data)
                << " sel=" << unsigned(stores_[i].sel) << "\n";
    std::exit(1);
  }

  void expect(const std::string &test, const std::string &field,
              uint32_t expected, uint32_t actual) {
    ++checks_;
    if (expected != actual)
      fail(test, field, expected, actual);
  }

  static bool uses_rs1(uint32_t instr) {
    switch (instr & 0x7f) {
    case 0x03:
    case 0x13:
    case 0x23:
    case 0x33:
    case 0x63:
    case 0x67:
      return true;
    default:
      return false;
    }
  }

  static bool uses_rs2(uint32_t instr) {
    switch (instr & 0x7f) {
    case 0x23:
    case 0x33:
    case 0x63:
      return true;
    default:
      return false;
    }
  }

  static uint8_t rs1(uint32_t instr) { return (instr >> 15) & 31; }
  static uint8_t rs2(uint32_t instr) { return (instr >> 20) & 31; }
  static uint8_t rd(uint32_t instr) { return (instr >> 7) & 31; }

  static bool is_addi(uint32_t instr, uint8_t dest, uint8_t source, int imm) {
    return (instr & 0x7f) == 0x13 && ((instr >> 12) & 7) == 0 &&
           rd(instr) == dest && rs1(instr) == source &&
           ((instr >> 20) & 0xfff) == (uint32_t(imm) & 0xfff);
  }

  static bool is_add(uint32_t instr, uint8_t dest, uint8_t a, uint8_t b) {
    return (instr & 0xfe00707f) == 0x33 && rd(instr) == dest &&
           rs1(instr) == a && rs2(instr) == b;
  }

  bool producer(const Snapshot &s, uint8_t source) const {
    if (source == 0)
      return false;
    return (s.idex_valid && s.idex_reg_we && s.idex_rd == source) ||
           (s.exmem_valid && s.exmem_reg_we && s.exmem_rd == source) ||
           (s.pending_valid && s.pending_reg_we && s.pending_rd == source) ||
           (s.writeback_valid && s.writeback_reg_we &&
            s.writeback_rd == source);
  }

  bool hazard(const Snapshot &s) const {
    if (!s.fetch_valid)
      return false;
    return (uses_rs1(s.fetch_instr) && producer(s, rs1(s.fetch_instr))) ||
           (uses_rs2(s.fetch_instr) && producer(s, rs2(s.fetch_instr)));
  }

  void record_observed_pc(const Snapshot &s) {
    if (!s.fetch_valid)
      return;
    if ((s.fetch_instr & 0x7f) == 0x17 && rd(s.fetch_instr) == 14 &&
        auipc_pc_ == 0)
      auipc_pc_ = s.fetch_pc;
    if ((s.fetch_instr & 0x7f) == 0x6f && rd(s.fetch_instr) == 1 &&
        jal_pc_ == 0)
      jal_pc_ = s.fetch_pc;
    if ((s.fetch_instr & 0x707f) == 0x67 && rd(s.fetch_instr) == 30 &&
        jalr_pc_ == 0)
      jalr_pc_ = s.fetch_pc;
  }

  void check_before_edge(const Snapshot &pre) {
    record_observed_pc(pre);
    if (pre.update_pc)
      saw_redirect_ = true;
    bool has_hazard = hazard(pre);
    if (has_hazard)
      expect("RAW hazard", "decode_ready", 0, pre.decode_ready);
    if (pre.fetch_valid && is_addi(pre.fetch_instr, 5, 4, 1) && has_hazard)
      saw_raw_rs1_hazard_ = true;
    if (pre.fetch_valid && is_add(pre.fetch_instr, 7, 0, 6) && has_hazard)
      saw_raw_rs2_hazard_ = true;
    if (pre.fetch_valid && is_addi(pre.fetch_instr, 24, 23, 1) && has_hazard) {
      saw_load_use_hazard_ = true;
      if (!pre.exmem_valid && pre.pending_valid && pre.pending_rd == 23 &&
          !pre.writeback_valid) {
        saw_load_pending_window_ = true;
        expect("load-use pending transition", "decode_ready", 0,
               pre.decode_ready);
      }
    }
    if (pre.fetch_valid && is_addi(pre.fetch_instr, 24, 23, 1) && !has_hazard &&
        pre.execute_ready) {
      saw_load_use_release_ = true;
      expect("load-use release", "decode_ready", 1, pre.decode_ready);
    }
    if (pre.fetch_valid && pre.decode_ready && !pre.update_pc && !pre.rst)
      ++accepted_pc_[pre.fetch_pc];
  }

  void check_after_edge(const Snapshot &pre, const Snapshot &post) {
    if (!pre.rst && !pre.update_pc && pre.fetch_valid && !pre.decode_ready) {
      expect("IF/ID stall", "valid", 1, post.fetch_valid);
      expect("IF/ID stall", "pc", pre.fetch_pc, post.fetch_pc);
      expect("IF/ID stall", "instruction", pre.fetch_instr, post.fetch_instr);
    }
    if (!pre.rst && !pre.update_pc && pre.idex_valid && !pre.execute_ready) {
      for (size_t i = 0; i < pre.idex_packet.size(); ++i)
        expect("ID/EX downstream stall", "packet word", pre.idex_packet[i],
               post.idex_packet[i]);
    }
    if (!pre.rst && pre.pending_valid && !pre.dwb_ack) {
      for (size_t i = 0; i < pre.pending_packet.size(); ++i)
        expect("pending LSU stability", "packet word", pre.pending_packet[i],
               post.pending_packet[i]);
      expect("Wishbone stability", "address", pre.dwb_address,
             post.dwb_address);
      expect("Wishbone stability", "data", pre.dwb_data, post.dwb_data);
      expect("Wishbone stability", "select", pre.dwb_sel, post.dwb_sel);
      expect("Wishbone stability", "write enable", pre.dwb_we, post.dwb_we);
    }
    if (!pre.rst && hazard(pre) && pre.execute_ready)
      expect("hazard bubble", "ID/EX valid", 0, post.idex_valid);
    if (!pre.rst && pre.fetch_valid && pre.decode_ready && !pre.update_pc) {
      if (is_addi(pre.fetch_instr, 5, 4, 1))
        ++raw_rs1_consumer_captures_;
      if (is_add(pre.fetch_instr, 7, 0, 6))
        ++raw_rs2_consumer_captures_;
      if (is_addi(pre.fetch_instr, 24, 23, 1))
        ++load_consumer_captures_;
    }
    if (!post.rst && post.pending_valid) {
      expect("pending LSU", "Wishbone cycle", 1, post.dwb_cyc);
      expect("pending LSU", "Wishbone strobe", 1, post.dwb_stb);
    }
    if (!post.pending_valid) {
      expect("no pending LSU", "Wishbone cycle", 0, post.dwb_cyc);
      expect("no pending LSU", "Wishbone strobe", 0, post.dwb_stb);
    }
    if (!pre.rst && pre.pending_valid && pre.dwb_ack) {
      expect("LSU ACK", "pending valid", 0, post.pending_valid);
      expect("LSU ACK", "memory writeback valid", 1, post.writeback_valid);
    }
  }

  void record_events(const Snapshot &pre) {
    if (!pre.rst && dut_.reg_wr_en) {
      uint8_t address = dut_.reg_wr_addr;
      uint32_t data = dut_.reg_wr_data;
      if (address != 0) {
        if (address == 1 &&
            std::none_of(writes_.begin(), writes_.end(),
                         [](const RegWrite &w) { return w.rd == 1; }))
          expect("smoke x1", "write data", 5, data);
        if (address == 2 &&
            std::none_of(writes_.begin(), writes_.end(),
                         [](const RegWrite &w) { return w.rd == 2; }))
          expect("smoke x2", "write data", 7, data);
        if (address == 3 &&
            std::none_of(writes_.begin(), writes_.end(),
                         [](const RegWrite &w) { return w.rd == 3; }))
          expect("smoke x3", "write data", 12, data);
        if (address == 12 &&
            std::none_of(writes_.begin(), writes_.end(),
                         [](const RegWrite &w) { return w.rd == 12; }))
          expect("x0 architectural behavior", "x12 write data", 5, data);
        if ((address == 27 && (data == 99 || data == 88 || data == 77)))
          fail("wrong-path register write", "x27", 0, data);
        registers_[address] = data;
        writes_.push_back({cycle_ + 1, address, data});
      }
    }
    if (!pre.rst && pre.dwb_cyc && pre.dwb_stb && pre.dwb_ack && pre.dwb_we) {
      stores_.push_back(
          {cycle_ + 1, pre.dwb_address, pre.dwb_data, pre.dwb_sel});
      if (pre.dwb_address == 0x1000) {
        if ((pre.dwb_data & 0xffff0000u) == 0xbad00000u)
          fail("program self-check", "failure signature", 0x600d600d,
               pre.dwb_data);
        expect("success signature", "select", 0xf, pre.dwb_sel);
        expect("success signature", "data", 0x600d600d, pre.dwb_data);
        finished_ = true;
      }
    }
  }

  void step() {
    dut_.clk = 0;
    dut_.eval();
    Snapshot pre = snapshot();
    check_before_edge(pre);
    record_events(pre);
    dut_.clk = 1;
    dut_.eval();
    ++cycle_;
    Snapshot post = snapshot();
    check_after_edge(pre, post);
  }

  bool has_write(uint8_t address, uint32_t data) const {
    return std::any_of(writes_.begin(), writes_.end(), [&](const RegWrite &w) {
      return w.rd == address && w.data == data;
    });
  }

  unsigned write_count(uint8_t address, uint32_t data) const {
    return static_cast<unsigned>(
        std::count_if(writes_.begin(), writes_.end(), [&](const RegWrite &w) {
          return w.rd == address && w.data == data;
        }));
  }

  bool has_store(uint32_t address, uint32_t data, uint8_t sel) const {
    return std::any_of(stores_.begin(), stores_.end(), [&](const Store &s) {
      return s.address == address && s.data == data && s.sel == sel;
    });
  }

  unsigned store_count(uint32_t address, uint32_t data, uint8_t sel) const {
    return static_cast<unsigned>(
        std::count_if(stores_.begin(), stores_.end(), [&](const Store &s) {
          return s.address == address && s.data == data && s.sel == sel;
        }));
  }

  void require_write(const std::string &test, uint8_t address, uint32_t data) {
    expect(test, "register write", 1, has_write(address, data));
  }

  void require_store(const std::string &test, uint32_t address, uint32_t data,
                     uint8_t sel) {
    expect(test, "store handshake", 1, has_store(address, data, sel));
    expect(test, "store count", 1, store_count(address, data, sel));
  }

  void verify_write_order(
      const std::vector<std::pair<uint8_t, uint32_t>> &expected) {
    size_t cursor = 0;
    for (const auto &item : expected) {
      while (cursor < writes_.size() && (writes_[cursor].rd != item.first ||
                                         writes_[cursor].data != item.second))
        ++cursor;
      if (cursor == writes_.size())
        fail("register write order", "missing ordered write", item.second, 0);
      ++checks_;
      ++cursor;
    }
  }

  void verify_results() {
    require_write("smoke x1", 1, 5);
    require_write("smoke x2", 2, 7);
    require_write("smoke x3", 3, 12);
    require_write("RAW rs1 x4", 4, 10);
    require_write("RAW rs1 x5", 5, 11);
    require_write("RAW rs2 x6", 6, 13);
    require_write("RAW rs2 x7", 7, 13);
    require_write("dependency chain x8", 8, 1);
    require_write("dependency chain x9", 9, 2);
    require_write("dependency chain x10", 10, 3);
    require_write("dependency chain x11", 11, 4);
    expect("dependency chain x8", "write count", 1, write_count(8, 1));
    expect("dependency chain x9", "write count", 1, write_count(9, 2));
    expect("dependency chain x10", "write count", 1, write_count(10, 3));
    expect("dependency chain x11", "write count", 1, write_count(11, 4));
    require_write("x0 architectural behavior", 12, 5);
    require_write("LUI", 13, 0x12345000);
    expect("AUIPC observation", "instruction PC present", 1, auipc_pc_ != 0);
    require_write("AUIPC", 14, auipc_pc_ + 0x1000);
    require_store("store data dependency", 0x2000, 42, 0xf);
    require_store("store address dependency", 0x2010, 55, 0xf);
    require_store("store-load", 0x2020, 77, 0xf);
    require_write("store-load", 20, 77);
    require_store("load-use setup", 0x2040, 41, 0xf);
    require_write("load-use load", 23, 41);
    require_write("load-use consumer", 24, 42);
    require_store("store byte lane 1", 0x2051, 0x00008000, 0x2);
    require_store("store byte lane 3", 0x2053, 0x80000000, 0x8);
    require_write("signed byte", 23, 0xffffff80);
    require_write("unsigned byte", 24, 0x80);
    require_store("store half lane 0", 0x2060, 0x00008001, 0x3);
    require_store("store half lane 2", 0x2062, 0x80010000, 0xc);
    require_write("signed half", 23, 0xffff8001);
    require_write("unsigned half", 24, 0x00008001);
    require_write("branch not taken", 26, 7);
    expect("branch taken wrong path", "x27", 0, registers_[27]);
    require_write("branch target", 28, 2);
    expect("branch target", "write count", 1, write_count(28, 2));
    expect("JAL observation", "instruction PC present", 1, jal_pc_ != 0);
    require_write("JAL link", 1, jal_pc_ + 4);
    require_write("JAL target", 2, 9);
    expect("JAL target", "write count", 1, write_count(2, 9));
    expect("JALR observation", "instruction PC present", 1, jalr_pc_ != 0);
    require_write("JALR link", 30, jalr_pc_ + 4);
    require_write("JALR target", 3, 21);
    expect("JALR target", "write count", 1, write_count(3, 21));
    expect("redirect", "observed", 1, saw_redirect_);
    expect("RAW rs1", "hazard observed", 1, saw_raw_rs1_hazard_);
    expect("RAW rs2", "hazard observed", 1, saw_raw_rs2_hazard_);
    expect("load-use", "hazard observed", 1, saw_load_use_hazard_);
    expect("load-use pending transition", "observed", 1,
           saw_load_pending_window_);
    expect("load-use release", "observed", 1, saw_load_use_release_);
    expect("RAW rs1 consumer", "capture count", 1, raw_rs1_consumer_captures_);
    expect("RAW rs2 consumer", "capture count", 1, raw_rs2_consumer_captures_);
    expect("load-use consumer", "capture count", 1, load_consumer_captures_);
    expect("architectural stores", "total count", 9,
           static_cast<uint32_t>(stores_.size()));
    verify_write_order({{31, 0x1000},
                        {1, 5},
                        {2, 7},
                        {3, 12},
                        {4, 10},
                        {5, 11},
                        {6, 13},
                        {7, 13},
                        {8, 1},
                        {9, 2},
                        {10, 3},
                        {11, 4},
                        {12, 5},
                        {13, 0x12345000},
                        {14, auipc_pc_ + 0x1000},
                        {20, 77},
                        {24, 42},
                        {26, 7},
                        {28, 2},
                        {1, jal_pc_ + 4},
                        {2, 9},
                        {30, jalr_pc_ + 4},
                        {3, 21}});
  }
};

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  Vroflan_v_wrapper dut;
  IntegrationTest test(dut);
  return test.run();
}
