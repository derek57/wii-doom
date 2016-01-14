cppcheck --enable=all --inconclusive --std=posix --force -I /usr/include/SDL/ . 2>&1 | tee cppcheck_logfile.txt

#cppcheck --enable=all --inconclusive --std=posix --force -I /usr/lib/gcc/i686-linux-gnu/4.9/include -I /usr/include/ -I /usr/local/include/ -I /home/user/WIIDOOM/opl/ -I /home/user/WIIDOOM/pcsound/ -I /home/user/WIIDOOM/src/ -I /home/user/WIIDOOM/src/doom/ -I /home/user/WIIDOOM/wii/ -U_TYPE_size_t -U_TYPE_wchar_t -U__INT16_TYPE__ -U__INT32_TYPE__ -U__INT64_TYPE__ -U__INT8_TYPE__ -U__INTPTR_TYPE__ -U__UINT16_TYPE__ -U__UINT32_TYPE__ -U__UINT64_TYPE__ -U__UINT8_TYPE__ -U__UINTPTR_TYPE__ -U_TYPE_ptrdiff_t -Ubasename -UPNG_EXPORT_LAST_ORDINAL . 2>&1 | tee cppcheck_logfile.txt

#Thanks to Fabian Greffrath (Crispy DOOM)
