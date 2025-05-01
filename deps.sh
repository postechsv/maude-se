#!/usr/bin/env bash

if [[ "$(uname -s)" == "Darwin" ]]; then
    brew install bison
    brew install flex

    echo 'export PATH="$(brew --prefix bison)/bin:$PATH"' >> $HOME/.bashrc
    echo 'export PATH="$(brew --prefix flex)/bin:$PATH"' >> $HOME/.bashrc
fi