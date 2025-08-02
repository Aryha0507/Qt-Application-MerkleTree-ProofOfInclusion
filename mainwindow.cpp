#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>

#include "merkletree.h"    // sha256, getMerkleTreeLevels()
#include <openssl/sha.h>   // only needed if you call sha256() here

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , netManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    connect(ui->pushButtonGenerate, &QPushButton::clicked,
            this, &MainWindow::handleGenerateTransactions);

    connect(ui->pushButtonBuild, &QPushButton::clicked,
            this, &MainWindow::handleBuildMerkle);

    connect(ui->pushButtonClear, &QPushButton::clicked,
            this, &MainWindow::handleClearFields);

    connect(ui->pushButtonFetch, &QPushButton::clicked,
            this, &MainWindow::handleFetchBlock);

    connect(ui->pushButtonVerify, &QPushButton::clicked,
            this, &MainWindow::handleVerifyLeaf);
}

MainWindow::~MainWindow() {
    delete ui;
}

// 1) Auto-generate dummy transactions Tx1…TxN
void MainWindow::handleGenerateTransactions() {
    int count = ui->spinBoxCount->value();
    QStringList lines;
    for (int i = 1; i <= count; ++i)
        lines << QString("Tx%1").arg(i);
    ui->plainTextEditInput->setPlainText(lines.join('\n'));
}

// 2) Build Merkle tree & display root + full levels
void MainWindow::handleBuildMerkle() {
    // 1) Read user input lines
    QStringList lines = ui->plainTextEditInput
                            ->toPlainText()
                            .split('\n', Qt::SkipEmptyParts);

    // 2) Convert to std::vector<string>
    std::vector<std::string> txs;
    for (const QString &ln : lines)
        txs.push_back(ln.toStdString());

    // 3) Build all levels of the Merkle tree
    auto levels = getMerkleTreeLevels(txs);

    // ─── 4) CLAMP THE VERIFY-LEAF SPINBOX ───────────────────────────────
    //    Only indices [0 .. leafCount-1] are valid
    int leafCount = levels.empty() ? 0 : int(levels[0].size());
    ui->spinBoxVerifyIndex->setMinimum(0);
    ui->spinBoxVerifyIndex->setMaximum(leafCount > 0 ? leafCount - 1 : 0);
    // ────────────────────────────────────────────────────────────────────

    // 5) Display the Merkle root
    if (!levels.empty() && !levels.back().empty()) {
        ui->labelOutput->setText(
            "Merkle Root: " +
            QString::fromStdString(levels.back()[0])
            );
    } else {
        ui->labelOutput->setText("No valid transactions!");
    }

    // 6) Display every level in the tree box
    QString treeText;
    for (int lvl = 0; lvl < (int)levels.size(); ++lvl) {
        treeText += QString("Level %1:\n").arg(lvl);
        for (const auto &h : levels[lvl])
            treeText += "  " + QString::fromStdString(h) + "\n";
        treeText += "\n";
    }
    ui->plainTextEditTree->setPlainText(treeText);
}


// 3) Clear all fields & reset spinbox
void MainWindow::handleClearFields() {
    ui->plainTextEditInput->clear();
    ui->labelOutput->clear();
    ui->plainTextEditTree->clear();
    ui->plainTextEditProof->clear();
    ui->spinBoxCount->setValue(4);
}

// 4) Fetch real Testnet block txids via REST, then rebuild tree
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
            ui->labelOutput->setText("Fetch error: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        // parse JSON array of txids
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) {
            ui->labelOutput->setText("Unexpected response format");
            reply->deleteLater();
            return;
        }

        // 1) Build a QStringList of the fetched txids
        QJsonArray arr = doc.array();
        QStringList txList;
        for (auto v : arr)
            txList << v.toString();

        // 2) Populate the input box
        ui->plainTextEditInput->setPlainText(txList.join('\n'));

        // ─── 3) CLAMP the Verify‐Leaf spin‐box ─────────────────────────
        int leafCount = txList.size();
        ui->spinBoxVerifyIndex->setMinimum(0);
        ui->spinBoxVerifyIndex->setMaximum(leafCount > 0 ? leafCount - 1 : 0);
        // ───────────────────────────────────────────────────────────────

        // 4) Now rebuild the Merkle Tree & display root + levels
        handleBuildMerkle();

        reply->deleteLater();
    });
}


// 5) Proof-of-inclusion: show sibling-hash path up to the root
void MainWindow::handleVerifyLeaf() {
    QStringList lines = ui->plainTextEditInput
                            ->toPlainText()
                            .split('\n', Qt::SkipEmptyParts);
    std::vector<std::string> txs;
    for (auto &ln : lines)
        txs.push_back(ln.toStdString());

    auto levels = getMerkleTreeLevels(txs);
    // ─── VALIDATE THE CHOSEN LEAF INDEX ────────────────────────────────
    int idx = ui->spinBoxVerifyIndex->value();
    int leafCount = levels.empty() ? 0 : static_cast<int>(levels[0].size());
    if (idx < 0 || idx >= leafCount) {
        ui->plainTextEditProof->setPlainText(
            QString("Invalid leaf index. Valid range is 0 to %1.")
                .arg(leafCount > 0 ? leafCount - 1 : 0)
            );
        return;
    }
    // ────────────────────────────────────────────────────────────────────

    // walk up collecting sibling hashes
    QString proof;
    std::string running = levels[0][idx];
    proof += "Leaf [" + QString::number(idx) + "]: " +
             QString::fromStdString(running) + "\n\n";

    for (int lvl = 0; lvl + 1 < (int)levels.size(); ++lvl) {
        auto &cur = levels[lvl];
        int sib = (idx % 2 == 0 ? idx + 1 : idx - 1);
        if (sib < 0 || sib >= (int)cur.size()) sib = idx;

        bool left = (sib < idx);
        std::string siblingHash = cur[sib];

        proof += QString("Level %1 %2-sibling: %3\n")
                     .arg(lvl)
                     .arg(left ? "L" : "R")
                     .arg(QString::fromStdString(siblingHash));

        if (left)
            running = sha256(siblingHash + running);
        else
            running = sha256(running + siblingHash);

        proof += "→ Parent: " +
                 QString::fromStdString(running) + "\n\n";

        idx = idx / 2;
    }

    // final verification
    QString root = QString::fromStdString(levels.back()[0]);
    if (running == levels.back()[0])
        proof += "✔️ Verified: root = " + root;
    else
        proof += "❌ Mismatch: got " + QString::fromStdString(running) +
                 ", expect " + root;

    ui->plainTextEditProof->setPlainText(proof);
}
