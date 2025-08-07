#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <vector>                   // for std::vector
#include <string>
#include <QComboBox>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QNetworkAccessManager;  // forward-declare

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleGenerateTransactions();
    void handleBuildMerkle();
    void handleClearFields();
    void handleFetchBlock();
    void handleVerifyLeaf();



private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *netManager;   // add this member
    QGraphicsScene *scene;
    QComboBox *comboBoxVerifyTxid;
    std::vector<std::vector<QGraphicsRectItem*>> nodeRects;
    void drawMerkleTreeGraphics(const std::vector<std::vector<std::string>>& levels);
};

#endif // MAINWINDOW_H
