curr_path=$(cd $(dirname $0) ; pwd)
project_path=$(cd ${curr_path}/.. ; pwd)

shared_lib_postfix="hbco"
shared_lib="lib${shared_lib_postfix}.so"
compiler="g++ -O3 -g"
link_flag="-L. -l${shared_lib_postfix} -lpthread -ldl"
include_flag="-I ${project_path}/source"
other_flag="-DDEBUG -fPIC $1"

source_path=${project_path}/source
cd ${source_path}
cpp_source=$(find . -maxdepth 1 -name "*.cpp")
asm_source=$(find . -maxdepth 1 -name "*.s")
${compiler} -c ${cpp_source} ${asm_source} ${include_flag} ${other_flag}
obj_source=$(find . -maxdepth 1 -name "*.o")
${compiler} -shared ${obj_source} -o ${project_path}/build/${shared_lib}
rm ${obj_source}
