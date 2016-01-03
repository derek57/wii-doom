cppcheck --enable=all --inconclusive --std=posix --force -I /usr/include/SDL/ . 2>&1 | tee cppcheck_logfile.txt

#Thanks to Fabian Greffrath (Crispy DOOM)
