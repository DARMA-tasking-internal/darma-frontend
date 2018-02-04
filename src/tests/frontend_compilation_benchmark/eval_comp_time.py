import os
import sys
import subprocess
import time
import jinja2
import json
from random import choice

#################################################

# Call cmake command and measure 
def call_cmake(command, source_dir, dir_path, target_name):
   start_time = time.time()
   subprocess.call([command,'--build', dir_path, '--target', target_name+'.measure'])
   finis_time = time.time()
   return finis_time - start_time


#################################################

# Store command line arguments
target    = sys.argv[1]
cmake_cmd = sys.argv[2]
darma_dir = sys.argv[3]

# Evaluate path for include_var.h
source_dir = os.path.dirname(os.path.abspath(os.path.dirname(__file__)))
source_dir = os.path.join(source_dir,'frontend_compilation_benchmark')

print(source_dir)

# Check if template exists. If so, evaluate template
if os.path.isfile(os.path.join(source_dir,target+'.jin.cc')):

   templateLD = jinja2.FileSystemLoader(searchpath=source_dir)
   templateEV = jinja2.Environment(loader=templateLD)

   time_dict = {}
   for i in range(1, 11, 5):

      # Sleep for one second
      time.sleep(1)

      # Render template without test section
      template = templateEV.get_template(target+'.jin.cc')
      outputText = template.render(N=i,teston=0,choice=choice)

      # Store source file
      source_file = open(os.path.join(source_dir,target+'.cc'), 'w')
      source_file.write(outputText)
      source_file.close() 

      # Evaluate compile time without test section
      compile_time_off = call_cmake(cmake_cmd, source_dir, darma_dir, target)

      # Sleep for one second
      time.sleep(1)

      # Render template with test section
      outputText = template.render(N=i,teston=1,choice=choice)

      # Store source file
      source_file = open(os.path.join(source_dir,target+'.cc'), 'w')
      source_file.write(outputText)
      source_file.close()

      # Evaluate compile time with test section
      compile_time_on = call_cmake(cmake_cmd, source_dir, darma_dir, target)

      # Save compile time in dictionary
      time_dict['N='+str(i)] = compile_time_on - compile_time_off

   file_object  = open(target+'.json', "w")
   file_object.write(json.dumps({'compilation time': time_dict}, indent=4, sort_keys=True, ensure_ascii=False))

else:

   print('Error: jinja2 template file '+target+'.jin.cc is missing.')
   sys.exit(errno.ENOENT)
