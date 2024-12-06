# folder containing the folder
model_folder="models"

# modes
modes=("collapsing" "no-collapsing")

# run Maude
function run_maude {
  local filename=$1
  local timeout_value=$2
  maude-se ${filename} -s yices
  # yices, z3, CVC4
  # timeout "$timeout_value fuck ${filename} -s yices"
}

# create a folder
function create_folder {
  local path=$1

  if [ ! -d "$path" ]; then
    mkdir -p "$path";
  fi
}

