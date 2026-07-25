#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"
mkdir -p build

run_unit() {
  local name="$1"
  local top="$2"
  shift 2
  local output="build/verilator_${name}"

  echo "==> ${name}"
  verilator --sv --relative-includes --cc --exe --build -j 0 \
    --top-module "$top" -Irtl --Mdir "$output" \
    "$@" "$ROOT_DIR/tests/${name}_tb.cpp" \
    -CFLAGS "-std=c++17" \
    -Wno-MODDUP -Wno-CASEINCOMPLETE -Wno-WIDTHTRUNC -Wno-WIDTHEXPAND
  "$output/V${top}"
}

run_unit alu alu rtl/alu.sv
run_unit control_flow control_flow rtl/control_flow.sv
run_unit fetch fetch_wrapper rtl/fetch.sv tests/fetch_wrapper.sv
run_unit decode decode_wrapper rtl/decode.sv tests/decode_wrapper.sv
run_unit execute execute_wrapper rtl/execute.sv tests/execute_wrapper.sv
run_unit memory memory_wrapper rtl/memory.sv tests/memory_wrapper.sv
run_unit writeback writeback_wrapper rtl/writeback.sv tests/writeback_wrapper.sv

echo "==> roflan_v integration"
verilator --sv --relative-includes --x-initial unique --x-assign unique \
  --cc --exe --build -j 0 --top-module roflan_v_wrapper -Irtl \
  --Mdir build/verilator_roflan_v \
  rtl/fetch_decode_pkg.sv \
  rtl/decode_execute_pkg.sv \
  rtl/execute_memory_pkg.sv \
  rtl/memory_writeback_pkg.sv \
  rtl/fetch.sv \
  rtl/decode.sv \
  rtl/alu.sv \
  rtl/control_flow.sv \
  rtl/execute.sv \
  rtl/memory.sv \
  rtl/writeback.sv \
  rtl/register_file.sv \
  rtl/roflan_v.sv \
  tests/roflan_v_wrapper.sv \
  "$ROOT_DIR/tests/roflan_v_tb.cpp" \
  -CFLAGS "-std=c++17" \
  -Wno-MODDUP -Wno-CASEINCOMPLETE -Wno-WIDTHTRUNC -Wno-WIDTHEXPAND

build/verilator_roflan_v/Vroflan_v_wrapper \
  +verilator+seed+1 \
  +verilator+rand+reset+2
