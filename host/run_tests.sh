#!/bin/bash
# Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
# Builds and runs the host tests (a computer only, no Cardputer).
set -e
cd "$(dirname "$0")"
OUT=$(mktemp -d)
g++ -std=c++17 -O2 -Wall -Wextra -o "$OUT/beat_test" beat_test.cpp
"$OUT/beat_test"
rm -rf "$OUT"
