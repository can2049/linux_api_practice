#include <curl/curl.h>
#include <curl/curlver.h>
#include <curl/easy.h>

#include <array>
#include <iostream>
#include <string>
#include <vector>

class NetworkChecker {
 public:
  NetworkChecker() { curl_global_init(CURL_GLOBAL_DEFAULT); }

  ~NetworkChecker() { curl_global_cleanup(); }

  // 检查网络连通性
  bool isNetworkAvailable() {
    // 知名服务器列表
    constexpr std::array<const char*, 5> servers = {
        "https://www.baidu.com", "https://www.bing.com",
        "https://www.google.com", "https://www.cloudflare.com",
        "https://www.github.com"};

    for (const auto& server : servers) {
      if (CheckSingleServer(server)) {
        std::cout << "Successfully connected to: " << server << std::endl;
        return true;
      }
      std::cerr << "Failed to connect to: " << server << std::endl;
    }

    return false;
  }

 private:
  bool CheckSingleServer(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
      std::cerr << "Failed to initialize CURL for: " << url << std::endl;
      return false;
    }

    // set up CURL options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);          // HEAD request only
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);         // 5 seconds timeout
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,
                     3L);  // 3 seconds connect timeout
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);

    // Disable SSL verification (only for testing, production should configure
    // CA certificates correctly)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    // Get HTTP response code (optional)
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    // Clean up resources
    curl_easy_cleanup(curl);

    // Check the result
    return (res == CURLE_OK) && (response_code == 200 || response_code == 301 ||
                                 response_code == 302);
  }

  // Static callback function
  static size_t writeCallback(void* buffer, size_t size, size_t nmemb,
                              void* userp) {
    (void)userp;          // Unused parameter
    (void)buffer;         // Unused parameter
    return size * nmemb;  // Just return the size of the data received
  }
};

int main() {
  NetworkChecker checker;

  if (checker.isNetworkAvailable()) {
    std::cout << "Network is available!" << std::endl;
    return 0;
  } else {
    std::cout << "Network is unavailable!" << std::endl;
    return 1;
  }
}
