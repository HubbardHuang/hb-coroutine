curr_path=$(cd $(dirname $0) ; pwd)
project_path=$(cd ${curr_path}/.. ; pwd)
script_path=${project_path}/build

source ${script_path}/color.sh

green_echo "✈ ✈ ✈ 正在生成 $1.exe"

blue_echo "[1/2] 编译 other/$1.cpp 生成 $1.exe"
g++ $2 ${project_path}/other/$1.cpp -o ${project_path}/$1.exe -lpthread
blue_echo "[2/2] 运行 example_$1.exe"
red_echo "=========================================="
sleep 2s
${project_path}/$1.exe $3