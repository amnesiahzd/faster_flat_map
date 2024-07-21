#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include "city.h"

// 生成随机字符串的函数
std::string GenerateRandomString(size_t length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += chars[rand() % chars.length()];
    }
    return result;
}

int main() {
    // 生成测试字符串
    std::vector<std::string> test_strings;
    for (int i = 0; i < 80000; ++i) {
        test_strings.push_back(GenerateRandomString(64)); // 生成长度为100的随机字符串
    }

    // 测试std::hash
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto& s : test_strings) {
        volatile auto hash = std::hash<std::string>{}(s);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "std::hash: " << diff.count() << " s\n";

    // 测试CityHash
    start = std::chrono::high_resolution_clock::now();
    for (const auto& s : test_strings) {
        volatile auto hash = CityHash64(s.data(), s.size());
    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff_city = end - start;
    std::cout << "CityHash: " << diff_city.count() << " s\n";
    std::cout << "accelarate: " << (diff.count() - diff_city.count()) / diff.count() * 100 << "%" << std::endl;

    return 0;
}
