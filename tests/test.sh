# run Maude
function run_maude {
  local filename=$1
  local timeout_value=$2
  maude-se ${filename} -s yices
}