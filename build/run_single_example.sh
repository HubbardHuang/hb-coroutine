curr_path=$(cd $(dirname $0) ; pwd)
project_path=$(cd ${curr_path}/.. ; pwd)
script_path=${project_path}/build

source ${script_path}/color.sh

chmod u+x ${script_path}/make_shared_lib.sh
chmod u+x ${script_path}/make_example_exe.sh

green_echo "✈ ✈ ✈ 正在生成 example_$1.exe"

blue_echo "[1/3] 将 source 目录中的文件编译生成动态链接库 libhbco.so"
${script_path}/make_shared_lib.sh "$2"
blue_echo "[2/3] 编译 test/example_$1.cpp 并链接 libhbco.so 生成 example_$1.exe"
${script_path}/make_example_exe.sh $1 "$2"
blue_echo "[3/3] 运行 example_$1.exe"
red_echo "=========================================="
sleep 2s
sudo nice -n -19 ${project_path}/example_$1.exe $3
