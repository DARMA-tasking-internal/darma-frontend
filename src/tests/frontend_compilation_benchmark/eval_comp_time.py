import os
import sys
import subprocess
import time
import json

def touch(fname, times=None):
    with open(fname, 'a'):
        os.utime(fname, times)

target    = sys.argv[1]
cmake_cmd = sys.argv[2]
cmake_dir = sys.argv[3]
darma_dir = sys.argv[4]

# Evaluate path for include_var.h
source_dir = os.path.dirname(os.path.abspath(os.path.dirname(__file__)))
source_dir = os.path.join(source_dir,'frontend_compilation_benchmark')
input_name = os.path.join(source_dir,'include_var.h')

# Write include_var.h -> TESTON 0
inc_file = open(input_name, "w")
inc_file.write("#define TESTON 0")
inc_file.close()

# Evaluate compile time without test section
touch(os.path.join(source_dir,target+'.cc'), None)
start_time_test_off = time.time()
subprocess.call([cmake_cmd,'--build',darma_dir,'--target',target+'.measure'])
finis_time_test_off = time.time()

# Sleep for one second
time.sleep(1)

# Write include_var.h -> TESTON 1
inc_file = open(input_name, "w")
inc_file.write("#define TESTON 1")
inc_file.close()

# Evaluate compile time with test section
touch(os.path.join(source_dir,target+'.cc'), None)
start_time_test_on = time.time()
subprocess.call([cmake_cmd,'--build',darma_dir,'--target',target+'.measure'])
finis_time_test_on = time.time()

time_elapsed = (finis_time_test_on - start_time_test_on) - (finis_time_test_off - start_time_test_off)
file_object  = open(target+'.json', "w")
file_object.write(json.dumps({'compilation time': time_elapsed}, ensure_ascii=False))
