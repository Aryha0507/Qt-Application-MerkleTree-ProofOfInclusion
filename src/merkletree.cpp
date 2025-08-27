#include "merkletree.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

// Computes SHA-256 hash of input string and returns hex representation
std::string sha256(const std::string &data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)data.c_str(),
           data.size(), hash);

    // Convert binary hash to hexadecimal string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2)
           << std::setfill('0') << (int)hash[i];

    return ss.str();
}

// Constructs Merkle tree levels from a list of transactions
std::vector<std::vector<std::string>>
getMerkleTreeLevels(const std::vector<std::string> &transactions) {
    std::vector<std::vector<std::string>> levels;
    if (transactions.empty()) return levels;

    // Step 1: Hash each transaction to create the leaf level
    std::vector<std::string> current;
    for (auto &tx : transactions)
        current.push_back(sha256(tx));

    // Step 2: Build tree upward until only the root remains
    while (!current.empty()) {
        levels.push_back(current);
        if (current.size() == 1) break;

        // If odd number of nodes, duplicate the last one
        if (current.size() % 2)
            current.push_back(current.back());

        // Step 3: Hash each adjacent pair to form the next level
        std::vector<std::string> next;
        for (size_t i = 0; i < current.size(); i += 2)
            next.push_back(sha256(current[i] + current[i + 1]));

        current = std::move(next); // Move to next level
    }

    return levels; // Return all levels, from leaves to root
}
