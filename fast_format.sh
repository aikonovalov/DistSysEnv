#!/usr/bin/env bash

set -e

find src -type f -name "*.h" -o -name "*.cpp" | xargs clang-format -i
