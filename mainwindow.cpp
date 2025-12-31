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
    resultDisplay->setText(translated);
    QtConcurrent::run([this, original, translated]() {
        QStringList templates = {"The word '%1' means '%2'.", "Use '%1' in a sentence.", "Explain '%1'."};
        QString example = templates.at(QRandomGenerator::global()->bounded(templates.size())).arg(original, translated);
        QMetaObject::invokeMethod(this, [this, example](){ exampleDisplay->setText(example); });
    });
    DatabaseManager::instance().addHistory(original, translated);
    historyModel->select();
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
