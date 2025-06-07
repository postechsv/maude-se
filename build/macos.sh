#!/usr/bin/env bash

# failed on any error 
set -euo pipefail

brew install bison
brew install flex

export PATH="$(brew --prefix bison)/bin:$PATH"
export PATH="$(brew --prefix flex)/bin:$PATH"

./build/build.sh prep
./build/build.sh deps
./build/build.sh build-maude
./build/build.sh build-maude-se