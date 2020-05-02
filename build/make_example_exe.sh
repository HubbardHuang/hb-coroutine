curr_path=$(cd $(dirname $0) ; pwd)
project_path=$(cd ${curr_path}/.. ; pwd)

lib_dir=${project_path}/build
example_dir=${project_path}/test
key_lib_name="hbco"

example_file="${example_dir}/example_$1.cpp"
compiler="g++"
other_flag="-DDEBUG -O3 -g -Wall -fPIC"
include_flag="-I ${project_path}/source"
link_flag="-L ${lib_dir} -l${key_lib_name} -lpthread -ldl"

${compiler} $2 ${example_file} -o ${project_path}/example_$1.exe ${include_flag} ${link_flag} ${other_flag}