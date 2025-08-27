#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QPointF>
#include <QBrush>
#include <QPen>
#include <algorithm>
#include <QMessageBox>
#include <QColor>

#include "merkletree.h"    // getMerkleTreeLevels()
#include <openssl/sha.h>   // sha256()

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , netManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    // Set up graphics scene
    scene = new QGraphicsScene(this);
    ui->graphicsViewTree->setScene(scene);

    // Wire up buttons
    connect(ui->pushButtonGenerate, &QPushButton::clicked,
            this, &MainWindow::handleGenerateTransactions);
    connect(ui->pushButtonBuild,    &QPushButton::clicked,
            this, &MainWindow::handleBuildMerkle);
    connect(ui->pushButtonClear,    &QPushButton::clicked,
            this, &MainWindow::handleClearFields);
    connect(ui->pushButtonFetch,    &QPushButton::clicked,
            this, &MainWindow::handleFetchBlock);
    connect(ui->pushButtonVerify,   &QPushButton::clicked,
            this, &MainWindow::handleVerifyLeaf);
}

MainWindow::~MainWindow() {
    delete ui;
}

// 1) Generateing dummy transactions Tx1…TxN
void MainWindow::handleGenerateTransactions() {
    int count = ui->spinBoxCount->value();
    QStringList lines;
    for (int i = 1; i <= count; ++i)
        lines << QString("Tx%1").arg(i);

// Populating both input box and combo box
    ui->plainTextEditInput->setPlainText(lines.join('\n'));
    ui->comboBoxVerifyTxid->clear();
    ui->comboBoxVerifyTxid->addItems(lines);
}

// 2) Building Merkle tree & display text + graphics
void MainWindow::handleBuildMerkle() {
    QStringList lines = ui->plainTextEditInput
                            ->toPlainText()
                            .split('\n', Qt::SkipEmptyParts);

    ui->comboBoxVerifyTxid->clear();
    ui->comboBoxVerifyTxid->addItems(lines);

    std::vector<std::string> txs;
    for (const QString &ln : lines)
        txs.push_back(ln.toStdString());

    auto levels = getMerkleTreeLevels(txs);

    if (!levels.empty() && !levels.back().empty()) {
        ui->labelOutput->setText(
            "Merkle Root: " +
            QString::fromStdString(levels.back()[0])
            );
    } else {
        ui->labelOutput->setText("No valid transactions!");
    }

    QString treeText;
    for (int lvl = 0; lvl < int(levels.size()); ++lvl) {
        treeText += QString("Level %1:\n").arg(lvl);
        for (const auto &h : levels[lvl])
            treeText += "  " + QString::fromStdString(h) + "\n";
        treeText += "\n";
    }
    ui->plainTextEditTree->setPlainText(treeText);

    drawMerkleTreeGraphics(levels);

    if (!lines.isEmpty())
        ui->comboBoxVerifyTxid->setCurrentIndex(0);
}

// 3) For Clearing all fields with clear button
void MainWindow::handleClearFields() {
    ui->plainTextEditInput->clear();
    ui->comboBoxVerifyTxid->clear();
    ui->labelOutput->clear();
    ui->plainTextEditTree->clear();
    ui->plainTextEditProof->clear();
    ui->spinBoxCount->setValue(4);
    scene->clear();
    nodeRects.clear();
    nodeTexts.clear();
}

// 4) Fetch Testnet block txids via REST & rebuild
void MainWindow::handleFetchBlock() {
    QString hash = ui->lineEditBlockHash->text().trimmed();
    if (hash.isEmpty()) {
        ui->labelOutput->setText("Enter a block hash first");
        return;
    }

    QUrl url(QString("https://blockstream.info/testnet/api/block/%1/txids").arg(hash));
    QNetworkReply *reply = netManager->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Fetch Error",
                                 "Could not fetch block data:\n" + reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) {
            ui->labelOutput->setText("Unexpected response format");
            reply->deleteLater();
            return;
        }

        QJsonArray arr = doc.array();
        QStringList txList;
        for (auto v : arr) txList << v.toString();
        ui->plainTextEditInput->setPlainText(txList.join('\n'));
        ui->comboBoxVerifyTxid->clear();
        ui->comboBoxVerifyTxid->addItems(txList);

        handleBuildMerkle();
        reply->deleteLater();
    });
}

// 5) Proof-of-inclusion: text proof + highlight
void MainWindow::handleVerifyLeaf() {
    QStringList lines = ui->plainTextEditInput
                            ->toPlainText()
                            .split('\n', Qt::SkipEmptyParts);

    if (lines.isEmpty()) {
        QMessageBox::information(this, "Nothing to verify",
                                 "Enter or fetch transactions, then click Build.");
        return;
    }
    if (nodeRects.empty()) {
        QMessageBox::information(this, "No tree yet",
                                 "Build the Merkle tree first (click Build).");
        return;
    }

    std::vector<std::string> txs;
    for (const QString &ln : lines)
        txs.push_back(ln.toStdString());

    auto levels = getMerkleTreeLevels(txs);

    int idx = ui->comboBoxVerifyTxid->currentIndex();
    int leafCount = levels.empty() ? 0 : int(levels[0].size());
    if (idx < 0 || idx >= leafCount) {
        QMessageBox::warning(this, "Invalid Selection",
                             QString("You must pick a transaction ID from the list (0…%1).")
                                 .arg(leafCount - 1));
        return;
    }

    QString proof;
    std::string running = levels[0][idx];
    proof += "Leaf [" + QString::number(idx) + "]: " +
             QString::fromStdString(running) + "\n\n";

    for (int lvl = 0; lvl + 1 < int(levels.size()); ++lvl) {
        auto &cur = levels[lvl];
        int sib = (idx % 2 == 0 ? idx + 1 : idx - 1);
        if (sib < 0 || sib >= int(cur.size())) sib = idx;

        bool left = sib < idx;
        std::string siblingHash = cur[sib];
        proof += QString("Level %1 %2-sibling: %3\n")
                     .arg(lvl)
                     .arg(left ? "L" : "R")
                     .arg(QString::fromStdString(siblingHash));

        if (left) running = sha256(siblingHash + running);
        else      running = sha256(running + siblingHash);

        proof += "→ Parent: " +
                 QString::fromStdString(running) + "\n\n";

        idx /= 2;
    }

    QString root = QString::fromStdString(levels.back()[0]);
    proof += (running == levels.back()[0])
                 ? "✔️ Verified: root = " + root
                 : "❌ Mismatch: got " + QString::fromStdString(running) +
                       ", expect " + root;
    ui->plainTextEditProof->setPlainText(proof);

    // Clear old highlights
    for (auto &levelRects : nodeRects) {
        for (auto *r : levelRects) {
            r->setBrush(Qt::NoBrush);
            r->setPen(QPen(Qt::black));
        }
    }
    // Reset all text colors to a readable default for the dark background
    for (auto &levelTexts : nodeTexts) {
        for (auto *t : levelTexts) {
            t->setDefaultTextColor(Qt::white);
        }
    }


    // For Highlighting the path
    int cur = ui->comboBoxVerifyTxid->currentIndex();
    std::vector<int> path = { cur };
    for (int lvl = 0; lvl + 1 < int(levels.size()); ++lvl) {
        cur /= 2;
        path.push_back(cur);
    }

    for (int lvl = 0; lvl < int(path.size()); ++lvl) {
        int idxOnLevel = path[lvl];
        if (lvl >= 0 && lvl < int(nodeRects.size()) &&
            idxOnLevel >= 0 && idxOnLevel < int(nodeRects[lvl].size())) {
            auto *rect = nodeRects[lvl][idxOnLevel];
            rect->setBrush(QBrush(Qt::white));
            rect->setPen(QPen(Qt::red, 2));
            nodeTexts[lvl][idxOnLevel]->setDefaultTextColor(Qt::black);
        }
    }

    // Highlight sibling nodes
    for (int lvl = 0; lvl + 1 < int(levels.size()); ++lvl) {
        int child = path[lvl];
        int sib = (child % 2 == 0) ? child + 1 : child - 1;
        if (sib < 0 || sib >= int(levels[lvl].size())) continue;
        if (lvl >= 0 && lvl < int(nodeRects.size()) &&
            sib >= 0 && sib < int(nodeRects[lvl].size())) {
            auto *sibRect = nodeRects[lvl][sib];
            sibRect->setBrush(QBrush(QColor(200, 230, 255)));
            sibRect->setPen(QPen(Qt::blue, 2));
            nodeTexts[lvl][sib]->setDefaultTextColor(Qt::black);
        }
    }
}

// Draw the Merkle tree in the QGraphicsScene
void MainWindow::drawMerkleTreeGraphics(
    const std::vector<std::vector<std::string>>& levels
    ) {
    scene->clear();

    int levelCount = int(levels.size());
    if (levelCount == 0) return;

    nodeRects.clear();
    nodeRects.resize(levelCount);
    nodeTexts.clear();
    nodeTexts.resize(levelCount);

    const int nodeW = 80, nodeH = 30, vGap = 60, hGap = 20;
    int maxNodes = 0;
    for (auto &lvl : levels)
        maxNodes = std::max(maxNodes, int(lvl.size()));

    scene->setSceneRect(0, 0,
                        maxNodes * (nodeW + hGap),
                        levelCount * (nodeH + vGap));

    std::vector<std::vector<QPointF>> pos(levelCount);

    for (int i = 0; i < levelCount; ++i) {
        int cnt = int(levels[i].size());
        int totalW = cnt * (nodeW + hGap) - hGap;
        int x0 = int((scene->width() - totalW) / 2);
        int y  = i * (nodeH + vGap);

        for (int j = 0; j < cnt; ++j) {
            int x = x0 + j * (nodeW + hGap);

            auto *rect = scene->addRect(x, y, nodeW, nodeH);
            nodeRects[i].push_back(rect);

            auto *ti = scene->addText(QString::fromStdString(levels[i][j]).left(8) + "...");
            ti->setPos(x + 4, y + 4);
            nodeTexts[i].push_back(ti);

            pos[i].push_back(QPointF(x + nodeW/2, y + nodeH));
        }
    }

    for (int i = 0; i + 1 < levelCount; ++i) {
        for (int j = 0; j < int(pos[i].size()); ++j) {
            auto c = pos[i][j];
            auto p = pos[i+1][ j/2 ];
            scene->addLine(c.x(), c.y(), p.x(), p.y());
        }
    }
}
