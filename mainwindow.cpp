#include "mainwindow.h"
#include "databasemanager.h"
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QRandomGenerator>
#include <QTimer>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMenu>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    DatabaseManager::instance().initDatabase();
    setupUI();
    initModel();
    translator = new Translator(this);

    connect(searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(searchEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);
    connect(translator, &Translator::finished, this, &MainWindow::onTranslationFinished);
    connect(translator, &Translator::errorOccurred, this, &MainWindow::onTranslationError);
    connect(historyList, &QListView::clicked, this, &MainWindow::onHistoryItemClicked);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(favButton, &QPushButton::clicked, this, &MainWindow::onFavClicked);
    connect(copyButton, &QPushButton::clicked, this, &MainWindow::onCopyClicked);
    connect(resetButton, &QPushButton::clicked, searchEdit, &QLineEdit::clear); // 第18次
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::onAboutClicked); // 第19次
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::onHelpClicked);

    this->setWindowTitle("智能翻译字典 - Qt6.9.2");
    this->resize(800, 600);
    statusBar()->showMessage("准备就绪");
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    QVBoxLayout *leftLayout = new QVBoxLayout();

    QHBoxLayout *searchBarLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchButton = new QPushButton("查询");
    favButton = new QPushButton("收藏");
    copyButton = new QPushButton("复制");
    resetButton = new QPushButton("重置"); // 第18次
    aboutButton = new QPushButton("关于"); // 第19次
    helpButton = new QPushButton("帮助");
    helpButton->setFixedWidth(60);
    searchBarLayout->addWidget(helpButton);

    searchBarLayout->addWidget(searchEdit);
    searchBarLayout->addWidget(searchButton);
    searchBarLayout->addWidget(favButton);
    searchBarLayout->addWidget(copyButton);
    searchBarLayout->addWidget(resetButton);
    searchBarLayout->addWidget(aboutButton);

    leftLayout->addLayout(searchBarLayout);
    leftLayout->addWidget(new QLabel("翻译结果:"));
    resultDisplay = new QTextEdit();
    resultDisplay->setReadOnly(true);
    leftLayout->addWidget(resultDisplay);
    leftLayout->addWidget(new QLabel("例句展示:"));
    exampleDisplay = new QTextEdit();
    exampleDisplay->setReadOnly(true);
    leftLayout->addWidget(exampleDisplay);

    tabWidget = new QTabWidget();
    historyList = new QListView();
    favoriteList = new QListView();
    favoriteList->setContextMenuPolicy(Qt::CustomContextMenu);
    tabWidget->addTab(historyList, "历史记录");
    tabWidget->addTab(favoriteList, "生词本");

    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(tabWidget);
    clearButton = new QPushButton("清空历史");
    clearButton->setStyleSheet("background-color: #ff3b30; color: white;");
    rightLayout->addWidget(clearButton);

    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addLayout(rightLayout, 1);

    this->setStyleSheet(R"(
        QMainWindow { background-color: #f5f5f7; }
        QLineEdit { padding: 8px; border: 1px solid #ccc; border-radius: 4px; }
        QPushButton { padding: 8px; background-color: #007aff; color: white; border-radius: 4px; }
    )");
}

void MainWindow::initModel() {
    historyModel = new QSqlTableModel(this);
    historyModel->setTable("history");
    historyModel->setSort(3, Qt::DescendingOrder);
    historyModel->select();
    historyList->setModel(historyModel);
    historyList->setModelColumn(1);

    favoriteModel = new QSqlTableModel(this);
    favoriteModel->setTable("favorites");
    favoriteModel->select();
    favoriteList->setModel(favoriteModel);
    favoriteList->setModelColumn(1);

    connect(favoriteList, &QListView::customContextMenuRequested, this, &MainWindow::onFavoriteContextMenu);
}

void MainWindow::onSearchClicked() {
    QString text = searchEdit->text().trimmed();
    if (text.isEmpty()) return;
    searchButton->setEnabled(false);
    translator->translate(text);
}

void MainWindow::onTranslationFinished(const QString &original, const QString &translated) {
    // 1. 显示翻译结果
    resultDisplay->setText(translated);
    statusBar()->showMessage("查询成功", 2000);

    // 2. 【多线程模块】：生成 11 个随机中英对照例句
    QtConcurrent::run([this, original, translated]() {
        QStringList templates = {
            "The word '%1' is very important in this context.\n(在这个语境下，'%2'这个词非常重要。)",
            "I don't know how to use '%1' in a sentence.\n(我不知道如何在句子中使用'%2'。)",
            "Could you please explain the meaning of '%1'?\n(你能解释一下'%2'的意思吗？)",
            "He wrote the word '%1' on the blackboard.\n(他在黑板上写下了'%2'这个词。)",
            "Is '%1' a common expression in English?\n('%2'是英语中的常用表达吗？)",
            "I found the definition of '%1' in the dictionary.\n(我在字典里找到了'%2'的定义。)",
            "The translation of '%1' can vary depending on the situation.\n('%2'的翻译可能会根据情况而有所不同。)",
            "Please repeat the word '%1' after me.\n(请跟我重复'%2'这个词。)",
            "She struggled to pronounce '%1' correctly.\n(她很难准确地读出'%2'的发音。)",
            "You should pay attention to the spelling of '%1'.\n(你应该注意'%2'的拼写。)",
            "This sentence is a perfect example of how to use '%1'.\n(这个句子是说明如何使用'%2'的完美例子。)"
        };

        int index = QRandomGenerator::global()->bounded(templates.size());
        QString example = templates.at(index).arg(original, translated);

        // 回到主线程更新 UI
        QMetaObject::invokeMethod(this, [this, example](){
            exampleDisplay->setText(example);
        }, Qt::QueuedConnection);
    });

    // 3. 【数据库模块】：写入历史记录
    if (DatabaseManager::instance().addHistory(original, translated)) {
        historyModel->select();
    }

    // 4. 恢复按钮状态
    searchButton->setEnabled(true);
}

void MainWindow::onTranslationError(const QString &error) {
    resultDisplay->setText("错误: " + error);
    searchButton->setEnabled(true);
}

void MainWindow::onHistoryItemClicked(const QModelIndex &index) {
    searchEdit->setText(historyModel->data(historyModel->index(index.row(), 1)).toString());
    resultDisplay->setText(historyModel->data(historyModel->index(index.row(), 2)).toString());
}

void MainWindow::onClearClicked() {
    if (DatabaseManager::instance().clearHistory()) historyModel->select();
}

void MainWindow::onFavClicked() {
    QString word = searchEdit->text().trimmed();
    QString trans = resultDisplay->toPlainText().trimmed();
    if (!word.isEmpty() && DatabaseManager::instance().addFavorite(word, trans)) favoriteModel->select();
}

void MainWindow::onFavoriteContextMenu(const QPoint &pos) {
    QModelIndex index = favoriteList->indexAt(pos);
    if (!index.isValid()) return;
    QMenu menu(this);
    QAction *del = menu.addAction("删除");
    QString word = favoriteModel->data(favoriteModel->index(index.row(), 1)).toString();
    connect(del, &QAction::triggered, this, [this, word]() {
        if (DatabaseManager::instance().removeFavorite(word)) favoriteModel->select();
    });
    menu.exec(favoriteList->mapToGlobal(pos));
}

void MainWindow::onCopyClicked() {
    QGuiApplication::clipboard()->setText(resultDisplay->toPlainText());
}

void MainWindow::onAboutClicked() {
    QMessageBox::about(this, "关于", "Qt智能翻译助手 v1.0\n作者: T2005AS");
}

void MainWindow::onHelpClicked() {
    QMessageBox::information(this, "使用帮助", "1. 输入文字点击查询即可翻译\n2. 点击收藏可存入生词本\n3. 右键生词本条目可删除");
}
