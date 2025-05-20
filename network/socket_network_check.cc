#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class NetworkTester {
 public:
  NetworkTester() = default;

  // test network connectivity
  bool test_network_connectivity() {
    // Known server list (domain and port)
    const std::vector<std::pair<std::string_view, int>> servers = {
        {"map.draicloud.com", 8080},         // onboard
        {"geelymap.draicloud.com", 9443},    // geelymap
        {"drmap.draicloud.com", 9090},       // drmap
        {"apigw-prlt.map.tencent.com", 80},  // tencent map
        {"www.baidu.com", 80},               // Baidu
        {"www.bing.com", 80},                // Bing
        {"www.google.com", 80},              // Google
        {"www.cloudflare.com", 80},          // Cloudflare
        {"www.github.com", 80},              // GitHub
        {"www.qq.com", 80}                   // QQ
    };

    // test all servers
    std::cout << "Testing network connectivity...\n";
    bool network_ok = false;
    for (const auto& [host, port] : servers) {
      if (test_single_server(host, port)) {
        std::cout << "Successfully connected to " << host << ":" << port
                  << ", network is up.\n";
        network_ok = true;
      } else {
        std::cerr << "Failed to connect to " << host << ":" << port << "\n";
      }
    }

    return network_ok;
  }

 private:
  // test single server connection
  bool test_single_server(const std::string_view host, int port) {
    std::cout << "Testing connection to " << host << ":" << port << "...\n";
    // resolve hostname
    const struct hostent* server = gethostbyname(host.data());
    if (!server) {
      std::cerr << "Failed to resolve hostname: " << host << "\n";
      return false;
    }

    // create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      std::cerr << "Failed to create socket\n";
      return false;
    }

    auto fd_closer = [](int* fd) {
      if (fd && *fd != -1) {
        // std::cout << "Closing file descriptor: " << *fd << std::endl;
        ::close(*fd);
      }
      delete fd;
    };
    // use std::unique_ptr to manage socket resource
    std::unique_ptr<int, decltype(fd_closer)> sockfd_lock(new int(sockfd),
                                                          fd_closer);

    // set timeout (3 seconds)
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                   sizeof(timeout))) {
      std::cerr << "Failed to set receive timeout\n";
      return false;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                   sizeof(timeout))) {
      std::cerr << "Failed to set send timeout\n";
      return false;
    }

    // prepare connection address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // try to connect
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
      std::cerr << "Failed to connect to " << host << ":" << port
                << ", error: " << std::strerror(errno) << "\n";
      return false;
    }

    return true;
  }
};

int main() {
  NetworkTester tester;

  if (tester.test_network_connectivity()) {
    std::cout << "Network is up and running!\n";
    return 0;
  } else {
    std::cout << "Network is down or unreachable!\n";
    return 1;
  }
}
