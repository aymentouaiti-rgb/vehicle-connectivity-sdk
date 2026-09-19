#!/usr/bin/env bash
# Orchestre les tests contractuels : demarre les simulateurs ELM327 requis
# par chaque suite de tests, execute ctest, puis nettoie -- que les tests
# passent ou echouent (trap EXIT).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

PIDS=()
cleanup() {
    for pid in "${PIDS[@]}"; do
        kill "$pid" 2>/dev/null || true
    done
    rm -f /tmp/vcsdk_ttyA /tmp/vcsdk_ttyB
}
trap cleanup EXIT

# Un simulateur TCP par suite de tests (ports alignes sur les tests contractuels).
python3 "$SCRIPT_DIR/fixtures/elm327_emulator.py" 35010 > /tmp/vcsdk_emulator_35010.log 2>&1 &
PIDS+=("$!")
python3 "$SCRIPT_DIR/fixtures/elm327_emulator.py" 35011 > /tmp/vcsdk_emulator_35011.log 2>&1 &
PIDS+=("$!")
python3 "$SCRIPT_DIR/fixtures/elm327_emulator.py" 35020 > /tmp/vcsdk_emulator_35020.log 2>&1 &
PIDS+=("$!")
python3 "$SCRIPT_DIR/fixtures/elm327_emulator.py" 35030 > /tmp/vcsdk_emulator_35030.log 2>&1 &
PIDS+=("$!")

# Paire de pseudo-terminaux pour SerialConnector (socat cree les deux bouts).
socat -d -d pty,raw,echo=0,link=/tmp/vcsdk_ttyA pty,raw,echo=0,link=/tmp/vcsdk_ttyB \
    > /tmp/vcsdk_socat.log 2>&1 &
PIDS+=("$!")
sleep 0.5
python3 "$SCRIPT_DIR/fixtures/elm327_emulator_serial.py" /tmp/vcsdk_ttyB \
    > /tmp/vcsdk_emulator_serial.log 2>&1 &
PIDS+=("$!")
export VCSDK_TEST_SERIAL_CLIENT_PATH=/tmp/vcsdk_ttyA

sleep 1  # laisser les simulateurs demarrer

cd "$BUILD_DIR"
ctest --output-on-failure
