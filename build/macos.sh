#!/usr/bin/env bash

# failed on any error
set -euo pipefail

top_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"$top_dir/build.sh" install-deps
"$top_dir/build.sh" wheel
