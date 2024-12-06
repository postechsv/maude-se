# run Maude
function run_maude {
  local filename=$1
  local timeout_value=$2
  maude-se ${filename} -s yices
  # yices, z3, CVC4
  # timeout "$timeout_value fuck ${filename} -s yices"
}