# Merkle Tree Visualization (Qt + C++)

Interactive Qt GUI to build Merkle Trees from transactions, fetch **Bitcoin Testnet** TX IDs, and verify **proof-of-inclusion** with path highlighting.

## ✨ Features
- Build Merkle Trees from dummy or Bitcoin **Testnet** TX IDs
- Text + graphical tree views (Qt GUI)
- Proof-of-inclusion with highlighted path
- Cross-platform (Windows/Linux/macOS)
- Project file: `MerkleTreeBackendQt.pro` (Qt 6, C++17)


## 🏗 Build (qmake)
```bash
qmake
make    # or jom/nmake on Windows
## 📁 Structure
src/        # C++ & Qt UI sources
examples/   # sample transaction lists
tests/      # optional unit tests
MerkleTreeBackendQt.pro
