#!/usr/bin/env bash

set -e

find . -type f -name "*.h" -o -name "*.cpp" | xargs clang-format -i
