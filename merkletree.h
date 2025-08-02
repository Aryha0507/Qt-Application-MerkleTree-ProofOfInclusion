#ifndef MERKLETREE_H
#define MERKLETREE_H

#include <string>
#include <vector>

std::string sha256(const std::string &data);
std::vector<std::vector<std::string>>
getMerkleTreeLevels(const std::vector<std::string> &transactions);

#endif // MERKLETREE_H
