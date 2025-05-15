#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

// using namespace std;

std::vector<long> get_cpu_times() {
  std::ifstream file("/proc/stat");
  std::string line;
  getline(file, line);
  std::istringstream iss(line);
  std::string cpu;
  std::vector<long> times;
  iss >> cpu;  // skip "cpu"
  long time;
  while (iss >> time) {
    times.push_back(time);
  }
  return times;
}

float calculate_cpu_usage() {
  auto t1 = get_cpu_times();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  auto t2 = get_cpu_times();

  long long total_diff =
      (t2[0] + t2[1] + t2[2] + t2[3] + t2[4] + t2[5] + t2[6] + t2[7]) -
      (t1[0] + t1[1] + t1[2] + t1[3] + t1[4] + t1[5] + t1[6] + t1[7]);

  long long idle_diff = (t2[3] + t2[4] - t1[3] - t1[4]);

  float usage = (float)(total_diff - idle_diff) / total_diff * 100.0f;
  return usage;
}

int main() {
  std::cout.precision(2);
  std::cout << std::fixed << "CPU Usage: " << calculate_cpu_usage() << "%"
            << std::endl;
  return 0;
}
