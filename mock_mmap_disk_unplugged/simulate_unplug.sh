#!/bin/bash
# simulate_unplug.sh

# 编译 C++ 程序
g++ -o2 mmap_unplug_simulate.cpp -o mmap_unplug_simulate

# 启动 mmap 写入程序（后台运行）
./mmap_unplug_simulate &
PID=$!

# 等待 3 秒，然后突然删除文件（模拟拔盘）
sleep 3
echo "Simulating disk unplug by removing the file!"
rm -f test_mmap.dat

sudo bash -c "echo 1 > /proc/sys/vm/drop_caches" || exit 1 

# 等待进程结束（应该会因 SIGBUS 崩溃）
wait $PID
echo "Process exited "

# 检查文件是否损坏
if [ -f "test_mmap.dat" ]; then
    echo "File still exists (may be corrupted)."
else
    echo "File was removed (simulating unplugged disk)."
fi
