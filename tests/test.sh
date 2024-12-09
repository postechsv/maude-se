# folder containing the folder
model_folder="models"

# modes
modes=("collapsing" "no-collapsing")
locations=("loc1" "loc2")

# run Maude
function run_maude {
  local filename=$1
  local timeout_value=$2
  local solver=$3
  maude-se ${filename} -s $solver
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
        run_maude "${new_file}" "${timeout}" > "${result_file}" 60 yices2
      fi
    done    
  done
}

# Checks if Maude and solvers are installed
function check_dependencies {
  local required_tools=("maude-se" "yices" "z3" "cvc4" "timeout")
  
  for tool in "${required_tools[@]}"; do
    if ! command -v "$tool" &> /dev/null; then
      echo "Error: '$tool' is not installed or not in PATH."
      return 1
    fi
  done
  
  echo "All dependencies are present."
  return 0
}


function cleanup_old_results {
  local log_dir=$1
  local days_old=${2:-7}

  if [ -d "$log_dir" ]; then
    find "$log_dir" -type f -mtime +$days_old -exec rm {} \;
    echo "Cleaned up result files older than $days_old days in $log_dir"
  else
    echo "Warning: $log_dir does not exist. No cleanup performed."
  fi
}


# Summarizes results from a directory into a CSV file
function summarize_results {
  local log_dir=$1
  local summary_file=${2:-"summary.csv"}

  echo "file_name,execution_time,exit_code" > "$summary_file"
  
  for file in "$log_dir"/*.res; do
    if [[ -f "$file" ]]; then
      # 예시 추출 로직: 결과 파일에서 실행 시간이나 exit code를 grep 등으로 추출
      # 실제 파일 포맷에 맞게 수정 필요
      local time_val=$(grep "Execution time:" "$file" | awk '{print $3}')
      local exit_code_val=$(grep "Exit code:" "$file" | awk '{print $3}')
      echo "$(basename "$file"),$time_val,$exit_code_val" >> "$summary_file"
    fi
  done
}

# Runs benchmarks in parallel using GNU parallel if available
function run_benchmarks_parallel {
  local file_list=("$@")
  
  if ! command -v parallel &> /dev/null; then
    echo "Warning: 'parallel' command not found. Running sequentially."
    for f in "${file_list[@]}"; do
      run_maude "$f" "60"
    done
  else
    echo "${file_list[@]}" | parallel -j4 run_maude {} 60
  fi
}

# Prints usage information
function print_usage {
  echo "Usage: $0 [options]"
  echo "Options:"
  echo "  -m <model>     Specify the model name"
  echo "  -t <timeout>   Specify the timeout in seconds"
  echo "  -h             Print this help message"
}

# 메인 로직: 인자 처리
if [[ "$1" == "-h" ]]; then
  print_usage
  exit 0
fi

check_dependencies || exit 1

# 예: 모델과 timeout을 인자로 받아서 run_benchmark 실행
model="${1:-default_model}"
timeout_value="${2:-60}"

run_benchmark "$model" "$timeout_value"