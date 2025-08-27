#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Include core Qt components and standard libraries
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QNetworkAccessManager>
#include <vector>
#include <string>
#include <QComboBox>
#include <QGraphicsTextItem>

// Namespace setup for Qt UI components
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Forward declaration for network manager
class QNetworkAccessManager;

// MainWindow class declaration, inherits from QMainWindow
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);  // Constructor
    ~MainWindow();                                    // Destructor

private slots:
    void handleGenerateTransactions();   // Slot to generate dummy transactions
    void handleBuildMerkle();            // Slot to build Merkle tree from input
    void handleClearFields();            // Slot to reset all fields/UI elements
    void handleFetchBlock();             // Slot to fetch Bitcoin Testnet txids
    void handleVerifyLeaf();             // Slot to verify transaction inclusion

private:
    Ui::MainWindow *ui;                            // Pointer to UI elements
    QNetworkAccessManager *netManager;             // Handles HTTP requests
    QGraphicsScene *scene;                         // Scene for drawing tree
    QComboBox *comboBoxVerifyTxid;                 // Dropdown to select txid for proof
    std::vector<std::vector<QGraphicsRectItem*>> nodeRects; // Rectangle shapes for nodes
    std::vector<std::vector<QGraphicsTextItem*>> nodeTexts; // Hash text inside each node
    void drawMerkleTreeGraphics(const std::vector<std::vector<std::string>>& levels); // Renders tree
};

#endif // MAINWINDOW_H
