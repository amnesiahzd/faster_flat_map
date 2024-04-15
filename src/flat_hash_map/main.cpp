#include "new_faster_hashtable.hpp"  // 包含您的哈希表实现
#include <unordered_map>
#include <iostream>
#include <random>
#include <chrono>

// 定义您的哈希表类型，假设这是模板参数的一种可能实现
using FasterHashTable = ddaof::flat_hash_map<int, int>;

// 测试参数
const size_t TEST_SIZE = 4000000;  // 测试元素数量
const int NUM_TESTS = 8;          // 测试次数

// 生成随机数的函数
std::vector<int> generate_random_data(size_t size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);

    std::vector<int> data(size);
    for (auto& elem : data) {
        elem = dis(gen);
    }
    return data;
}

// 测试插入性能的函数
template <typename MapType>
long long test_insert_performance(const std::vector<int>& data) {
    auto start = std::chrono::high_resolution_clock::now();

    MapType map;
    for (auto& elem : data) {
        map[elem] = elem;
    }

    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

// 测试查找性能的函数
template <typename MapType>
long long test_lookup_performance(const std::vector<int>& data, MapType& map) {
    auto start = std::chrono::high_resolution_clock::now();

    for (auto& elem : data) {
        auto it = map.find(elem);
        if (it == map.end()) {
            // std::cerr << "Error: Element not found." << std::endl;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

int main() {
    // 生成随机数据
    auto data = generate_random_data(TEST_SIZE);

    long long total_insert_time_faster = 0;
    long long total_lookup_time_faster = 0;
    long long total_insert_time_std = 0;
    long long total_lookup_time_std = 0;

    FasterHashTable faster_map;
    std::unordered_map<int, int> std_map;

    for (int i = 0; i < NUM_TESTS; ++i) {
        // 测试自定义哈希表
        total_insert_time_faster += test_insert_performance<FasterHashTable>(data);
        total_lookup_time_faster += test_lookup_performance(data, faster_map);

        // 测试标准库哈希表
        total_insert_time_std += test_insert_performance<std::unordered_map<int, int>>(data);
        total_lookup_time_std += test_lookup_performance(data, std_map);
    }

    std::cout << "Average insert time (FasterHashTable): " << total_insert_time_faster / NUM_TESTS << " ms\n";
    std::cout << "Average lookup time (FasterHashTable): " << total_lookup_time_faster / NUM_TESTS << " ms\n";
    std::cout << "Average insert time (std::unordered_map): " << total_insert_time_std / NUM_TESTS << " ms\n";
    std::cout << "Average lookup time (std::unordered_map): " << total_lookup_time_std / NUM_TESTS << " ms\n";

    return 0;
}
