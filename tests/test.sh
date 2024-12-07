# folder containing the folder
model_folder="models"

# modes
modes=("collapsing" "no-collapsing")
locations=("loc1" "loc2")

# run Maude
function run_maude {
  local filename=$1
  local timeout_value=$2
  maude-se ${filename} -s yices
  # yices, z3, CVC4
  # timeout "$timeout_value fuck ${filename} -s yices"
  if [[ -z "$timeout_value" ]]; then
    timeout_value=60 # 기본 60초
  fi

  # timeout 사용해서 maude 명령어 실행
  if timeout "${timeout_value}s" maude-se "${filename}" -s yices; then
    return 0
  else
    echo "Error: Maude execution timed out or failed on $filename"
    return 1
  fi  
}

# create a folder
function create_folder {
  local path=$1

  if [ ! -d "$path" ]; then
    mkdir -p "$path";
  fi
}

# Todo refactoring
function parser_files {
  local model=$1
  local mode=$2
  cp "${model_folder}/${model}.${mode}.maude.template" "${model_folder}/${model}.${mode}.maude"
}

function run_benchmark {
  local model=$1
  local timeout=$2

  output_folder="logs/${model}"
  create_folder "${output_folder}"
  cp "${model_folder}/pta-base.maude" "${model_folder}/meta-pta.maude" "${output_folder}"

  for mode in ${modes[@]}; do
    parser_files "$model" "$mode"
  done

  for l in ${locations[@]}; do
    echo "reaching location: $l"

    for mode in ${modes[@]}; do
      maude_file="${model}.${mode}.maude"
      new_file="${output_folder}/${maude_file}.${l}"
      
      # 파일명에 timestamp 추가
      timestamp=$(date +%Y%m%d_%H%M%S)
      result_file="${new_file}.${timestamp}.res"

      if [[ ! -f "${new_file}" ]]; then
        sed "s/<replace>/${l}/" "${model_folder}/${maude_file}" > "${new_file}"
        echo "Running Maude with ${new_file}"
        run_maude "${new_file}" "${timeout}" > "${result_file}"
      fi
    done    
  done
}
