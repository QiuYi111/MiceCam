#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main() {
    std::cout
        << "{\"type\":\"hello\",\"detail\":\"Recording worker ready.\",\"previewAvailable\":false,"
           "\"previewMode\":\"offline\",\"previewDetail\":\"Preview is offline until recording starts.\"}"
        << std::endl;

    std::string line;
    if (std::getline(std::cin, line)) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    return 0;
}
