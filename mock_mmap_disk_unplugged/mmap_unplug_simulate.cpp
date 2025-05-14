// mmap_unplug_simulate.cpp

#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

// 处理 SIGBUS 信号（拔盘或 I/O 错误时触发）
void sigbus_handler(int sig) {
  std::cerr << "SIGBUS received! Disk may be unplugged or corrupted."
            << std::endl;
  exit(1);
}

int main() {
  const char* file_path = "test_mmap.dat";
  const size_t file_size = 1024 * 1024;  // 1MB

  // 1. 创建并扩展文件
  int fd = open(file_path, O_RDWR | O_CREAT, 0666);
  if (fd == -1) {
    perror("open failed");
    return 1;
  }
  ftruncate(fd, file_size);  // 调整文件大小

  // 2. 映射文件到内存
  char* mapped_data = (char*)mmap(nullptr, file_size, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, fd, 0);
  if (mapped_data == MAP_FAILED) {
    perror("mmap failed");
    close(fd);
    return 1;
  }

  // 3. 注册 SIGBUS 信号处理器
  signal(SIGBUS, sigbus_handler);

  std::cout << "Writing data via mmap... (Press Ctrl+C to simulate unplug)"
            << std::endl;

  // 4. 模拟持续写入（每隔 1s 写入一次）
  for (int i = 0; i < 10; i++) {
    std::string data = "Block " + std::to_string(i) + "\n";
    memcpy(mapped_data + (i * 128), data.c_str(), data.size());  // 写入数据
    msync(mapped_data, file_size, MS_SYNC);  // 强制同步到磁盘
    std::cout << "Written: " << data;
    sleep(1);
  }

  // 5. 清理
  munmap(mapped_data, file_size);
  close(fd);
  std::cout << "Done." << std::endl;
  return 0;
}
