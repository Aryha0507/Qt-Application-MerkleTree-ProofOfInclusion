#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
};

#endif // MAINWINDOW_H
