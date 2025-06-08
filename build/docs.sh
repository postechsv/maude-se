#!/usr/bin/env bash

# Settings/Utilities
# ------------------

# failed on any error
set -euo pipefail

# set variables
top_dir="$(pwd)"
doc_dir="$(pwd)/docs"

build() {
  cd $doc_dir
  make clean

  rm -rf ./source/assets/
  mkdir -p ./source/assets/maude-se-examples

  cp $top_dir/examples/smt-*.maude ./source/assets/maude-se-examples

  cd $doc_dir/source/assets

  zip -r maude-se-examples.zip maude-se-examples
  rm -rf maude-se-examples

  cp $top_dir/examples/coffee.maude $top_dir/examples/euf-ex.maude .

  cd $doc_dir
  make html
}

publish() {
  rm -rf $doc_dir/build/maude-se.github.io
  cd $doc_dir/build/
  git clone git@github.com:maude-se/maude-se.github.io.git
  cd maude-se.github.io
  touch .nojekyll
  cp -r ../html/* ./
}

build_command="$1"
shift
case "$build_command" in
build) build "$@" ;;
publish) publish "$@" ;;

*) echo "
    usage: $0 [prep|build]
           $0 script <options>

    $0 prep     :   prepare Webpage repo
    $0 build    :   build
" ;;
esac
