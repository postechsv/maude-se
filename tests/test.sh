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

function run_benchmark {
  local model=$1
  local timeout=$2

  # create folder to save the output of the model
  output_folder="logs/${model}"
  create_folder "${output_folder}"
  cp "${model_folder}/pta-base.maude" "${model_folder}/meta-pta.maude" "${output_folder}"

  # 1) generate the full model with parser
  for mode in ${modes[@]}; do
    parser_files $model "$mode"
  done
}
