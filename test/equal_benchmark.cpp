#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// 假设这是你的自定义比较函数
bool customStringEqual(const std::string& lhs, const std::string& rhs) {
    for (uint16_t i = 9; i < 58; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

// 生成随机字符串
std::vector<std::string> generateRandomStrings(size_t numStrings, size_t totalLength) {
    if (totalLength <= 15) { // 确保总长度足以包含固定的前缀和后缀
        throw std::invalid_argument("Total length must be greater than 15 to accommodate prefix and suffix.");
    }

    std::vector<std::string> strings;
    strings.reserve(numStrings); // 预分配所需空间

    const std::string prefix = "AAACPGC";
    const std::string suffix = "VVQTYPXX";
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::mt19937 rng(std::random_device{}()); // 随机数生成器
    std::uniform_int_distribution<std::string::size_type> charDist(0, chars.size() - 1);
    std::bernoulli_distribution repeatDist(0.0f); // 35%概率决定是否重复

    size_t middleLength = totalLength - prefix.length() - suffix.length(); // 计算中间部分的长度

    std::string lastString = prefix + std::string(middleLength, ' ') + suffix; // 初始化lastString以避免未定义行为
    // 生成第一个字符串的中间部分
    for (size_t j = 0; j < middleLength; ++j) {
        lastString[prefix.length() + j] = chars[charDist(rng)];
    }
    strings.push_back(lastString);

    for (size_t i = 1; i < numStrings; ++i) {
        if (!repeatDist(rng)) { // 65%概率生成新字符串
            for (size_t j = 0; j < middleLength; ++j) {
                lastString[prefix.length() + j] = chars[charDist(rng)];
            }
        }
        // 否则，lastString保持不变，即重复上一个字符串

        strings.push_back(lastString);
    }

    return strings;
}

int main() {
    const size_t numberOfStrings = 1000000;
    const size_t stringLength = 64;
    std::vector<std::string> strings = generateRandomStrings(numberOfStrings, stringLength);

    // 自定义比较函数的基准测试
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < numberOfStrings - 1; ++i) {
        customStringEqual(strings[i], strings[i + 1]);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> customDuration = end - start;
    std::cout << "Custom string equal duration: " << customDuration.count() << " ms\n";

    // std::equal的基准测试
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < numberOfStrings - 1; ++i) {
        std::equal(strings[i].begin(), strings[i].end(), strings[i + 1].begin());
    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> stdDuration = end - start;
    std::cout << "std::equal duration: " << stdDuration.count() << " ms\n";

    return 0;
}
