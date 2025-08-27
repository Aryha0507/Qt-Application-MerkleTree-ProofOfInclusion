#ifndef MERKLETREE_H
#define MERKLETREE_H

// Standard library includes
#include <string>
#include <vector>

// Computes the SHA-256 hash of a given string
std::string sha256(const std::string &data);

// Builds the Merkle tree from a list of transactions and returns all levels
std::vector<std::vector<std::string>>
getMerkleTreeLevels(const std::vector<std::string> &transactions);

#endif // MERKLETREE_H
