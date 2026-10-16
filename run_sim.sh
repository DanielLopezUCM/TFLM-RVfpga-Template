#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../../.." && pwd -P)"
RV_ROOT="${RV_ROOT:-$REPO_ROOT}"
TFLM_DIR="${TFLM_DIR:-$SCRIPT_DIR/tflm}"
BUILD_ROOT="${BUILD_DIR:-$SCRIPT_DIR/build}"
VEER_TARGET="${VEER_TARGET:-default}"
MAX_CYCLES="${MAX_CYCLES:-20000000}"
VERILATOR_CMD="${VERILATOR:-verilator}"

usage() {
    cat <<'EOF'
Usage: ./run_sim.sh [--help] [-- simulator-arguments]

Build the bare-metal TFLM ELF, convert it to program.hex, build the VeeR
Verilator testbench, and run the simulation. Simulator arguments after --
are forwarded to Vtb_top.

Environment:
  RV_ROOT       VeeR repository root (detected from this script by default)
  TFLM_DIR      TensorFlow Lite Micro source tree (defaults to ./tflm)
  BUILD_DIR     isolated simulation output directory (defaults to ./build)
  VEER_TARGET   VeeR configuration target (defaults to default)
  MAX_CYCLES    override the testbench cycle limit (defaults to 20000000)
  VERILATOR     Verilator executable (defaults to verilator)
EOF
}

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    usage
    exit 0
fi
if [[ "${1:-}" == "--" ]]; then
    shift
fi

if [[ "$RV_ROOT" != /* ]]; then
    RV_ROOT="$SCRIPT_DIR/$RV_ROOT"
fi
RV_ROOT="$(cd -- "$RV_ROOT" && pwd -P)"
if [[ "$BUILD_ROOT" != /* ]]; then
    BUILD_ROOT="$SCRIPT_DIR/$BUILD_ROOT"
fi
if [[ ! -d "$TFLM_DIR" ]]; then
    printf 'TFLM source directory not found: %s\n' "$TFLM_DIR" >&2
    exit 1
fi
if [[ ! -f "$RV_ROOT/tools/Makefile" || ! -f "$RV_ROOT/configs/veer.config" ]]; then
    printf 'RV_ROOT is not a VeeR repository: %s\n' "$RV_ROOT" >&2
    exit 1
fi
if [[ -n "$MAX_CYCLES" ]]; then
    if [[ ! "$MAX_CYCLES" =~ ^[1-9][0-9]*$ ]]; then
        printf 'MAX_CYCLES must be a positive integer.\n' >&2
        exit 1
    fi
    VERILATOR_CMD+=" -GMAX_CYCLES=$MAX_CYCLES"
fi

for tool in make riscv64-unknown-elf-g++ riscv64-unknown-elf-gcc \
            riscv64-unknown-elf-objcopy g++ "${VERILATOR_CMD%% *}"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'Required tool not found in PATH: %s\n' "$tool" >&2
        exit 1
    fi
done

mkdir -p "$BUILD_ROOT"
SNAPSHOT="tflm-$VEER_TARGET"

# Rebuild the harness so configuration and optional cycle-limit changes apply.
rm -rf -- "$BUILD_ROOT/obj_dir" "$BUILD_ROOT/$SNAPSHOT"
rm -f -- "$BUILD_ROOT/verilator-build"

printf '[1/4] Building bare-metal TFLM firmware...\n'
make -C "$SCRIPT_DIR" -f "$SCRIPT_DIR/Makefile" TFLM_DIR="$TFLM_DIR"

printf '[2/4] Generating Verilator program image...\n'
riscv64-unknown-elf-objcopy -O verilog \
    "$SCRIPT_DIR/mnist_riscv_test.elf" "$BUILD_ROOT/program.hex"

printf '[3/4] Building VeeR Verilator testbench...\n'
make -C "$BUILD_ROOT" -f "$RV_ROOT/tools/Makefile" \
    RV_ROOT="$RV_ROOT" \
    BUILD_DIR="$SNAPSHOT" \
    target="$VEER_TARGET" \
    TEST=tflm \
    VERILATOR="$VERILATOR_CMD" \
    verilator-build

printf '[4/4] Running simulation...\n'
set +e
(
    cd -- "$BUILD_ROOT"
    ./obj_dir/Vtb_top "$@"
) 2>&1 | tee "$BUILD_ROOT/simulation.log"
sim_status=${PIPESTATUS[0]}
set -e

if (( sim_status != 0 )); then
    printf 'Verilator exited with status %d. See %s/simulation.log\n' \
        "$sim_status" "$BUILD_ROOT" >&2
    exit "$sim_status"
fi
if ! grep -q 'TEST_PASSED' "$BUILD_ROOT/simulation.log"; then
    printf 'Simulation ended without TEST_PASSED. Check the cycle limit and %s/simulation.log\n' \
        "$BUILD_ROOT" >&2
    exit 1
fi

printf 'Simulation passed. Logs and build outputs are in %s\n' "$BUILD_ROOT"