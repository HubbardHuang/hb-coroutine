compiler="g++"

shared_lib_postfix='syscallhook'
shared_lib="lib${shared_lib_postfix}.so"
link_flag="-L. -l${shared_lib_postfix} -lpthread -ldl"
echo ${link_flag}
include_flag="-I."
other_flag="-DDEBUG -fPIC"

executable="hbco.a"

cpp_source=$(find . -maxdepth 1 -name "*.cpp")
asm_source=$(find . -maxdepth 1 -name "*.s")
echo ${cpp_source}
${compiler} -g -c ${cpp_source} ${asm_source} ${include_flag} ${other_flag}
${compiler} -g -shared ${shared_lib_postfix}.o -o ${shared_lib}
obj_source=$(find . -maxdepth 1 -name "*.o" | grep -v "${shared_lib_postfix}.o")
echo ${obj_source}
${compiler} -g ${obj_source} -o ${executable} ${link_flag}

# Remember execute the following:
# export LD_LIBRARY_PATH=/where/you/install/lib:$LD_LIBRARY_PATH
# sudo ldconfig