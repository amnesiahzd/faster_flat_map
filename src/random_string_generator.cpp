#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

std::string generateRandomString(size_t length, const std::string& prefix, const std::string& suffix) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<std::string::size_type> dist(0, chars.size() - 1);

    std::string middle;
    for (size_t i = 0; i < length; ++i) {
        middle += chars[dist(rng)];
    }
    return prefix + middle + suffix;
}

void generateAndWriteStrings(const std::string& filename, size_t numStrings) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing." << std::endl;
        return;
    }

    const std::string prefix = "AAACPGC";
    const std::string suffix = "VVQTYPXX";
    const size_t middleLength = 49; // Total 64 characters, minus the length of prefix and suffix

    std::string previousString = generateRandomString(middleLength, prefix, suffix);
    outFile << previousString << "\n";

    std::mt19937 rng(std::random_device{}());
    std::bernoulli_distribution dist(0.35); // 35% probability

    for (size_t i = 1; i < numStrings; ++i) {
        if (dist(rng)) { // 35% chance to repeat the previous string
            outFile << previousString << "\n";
        } else {
            previousString = generateRandomString(middleLength, prefix, suffix);
            outFile << previousString << "\n";
        }
    }

    outFile.close();
}

int main() {
    generateAndWriteStrings("random_strings.txt", 300000);
    return 0;
}
