#include "mainwindow.h"
#include "databasemanager.h" // 引用数据库管理类
#include <QtConcurrent>
#include <QRandomGenerator>
#include <QFutureWatcher>
#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 1. 初始化数据库
    DatabaseManager::instance().initDatabase();

    // 2. 初始化UI
    setupUI();

    // 3. 初始化Model/View
    initModel();

    // 初始化翻译器
    translator = new Translator(this);

    connect(historyList, &QListView::clicked, this, &MainWindow::onHistoryItemClicked);

    // 连接信号与槽
    connect(searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(searchEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked); // 回车也能查
    connect(translator, &Translator::finished, this, &MainWindow::onTranslationFinished);
    connect(translator, &Translator::errorOccurred, this, &MainWindow::onTranslationError);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(copyButton, &QPushButton::clicked, this, &MainWindow::onCopyClicked);

    this->setWindowTitle("智能翻译字典 - Qt6.9.2");
    this->resize(800, 600);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    QVBoxLayout *leftLayout = new QVBoxLayout();

    // 搜索栏增加收藏按钮
    QHBoxLayout *searchBarLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchButton = new QPushButton("查询");
    favButton = new QPushButton("收藏"); // 新增
    favButton->setFixedWidth(60);

    searchBarLayout->addWidget(searchEdit);
    searchBarLayout->addWidget(searchButton);
    searchBarLayout->addWidget(favButton); // 新增

    copyButton = new QPushButton("复制结果");
    copyButton->setFixedWidth(80); // 设置一下宽度
    searchBarLayout->addWidget(copyButton);

    leftLayout->addLayout(searchBarLayout);
    leftLayout->addWidget(new QLabel("翻译结果:"));
    resultDisplay = new QTextEdit();
    resultDisplay->setReadOnly(true);
    leftLayout->addWidget(resultDisplay);

    leftLayout->addWidget(new QLabel("例句展示:"));
    exampleDisplay = new QTextEdit();
    exampleDisplay->setReadOnly(true);
    leftLayout->addWidget(exampleDisplay);

    // 右侧改为页签布局（双表展示）
    tabWidget = new QTabWidget();
    historyList = new QListView();
    favoriteList = new QListView();
    favoriteList->setContextMenuPolicy(Qt::CustomContextMenu);

    tabWidget->addTab(historyList, "历史记录");
    tabWidget->addTab(favoriteList, "生词本");

    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(new QLabel("数据管理:"));
    rightLayout->addWidget(tabWidget);

    clearButton = new QPushButton("清空历史");
    clearButton->setStyleSheet("background-color: #ff3b30; color: white;");
    rightLayout->addWidget(clearButton);

    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addLayout(rightLayout, 1);

    // 样式表更新
    this->setStyleSheet(R"(
        QMainWindow { background-color: #f5f5f7; }
        QLineEdit { padding: 8px; border: 1px solid #ccc; border-radius: 4px; }
        QPushButton { padding: 8px; background-color: #007aff; color: white; border-radius: 4px; }
        QTabWidget::pane { border: 1px solid #ddd; }
        QTabBar::tab { padding: 8px 20px; }
    )");
}

void MainWindow::initModel() {
    // 历史记录模型
    historyModel = new QSqlTableModel(this);
    historyModel->setTable("history");
    historyModel->setSort(3, Qt::DescendingOrder);
    historyModel->select();
    historyList->setModel(historyModel);
    historyList->setModelColumn(1);

    // 生词本模型 (第二张表)
    favoriteModel = new QSqlTableModel(this);
    favoriteModel->setTable("favorites");
    favoriteModel->select();
    favoriteList->setModel(favoriteModel);
    favoriteList->setModelColumn(1);

    // 连接收藏按钮信号
    connect(favButton, &QPushButton::clicked, this, &MainWindow::onFavClicked);
     // 连接右键取消收藏按钮信号
    connect(favoriteList, &QListView::customContextMenuRequested, this, &MainWindow::onFavoriteContextMenu);
}

void MainWindow::onSearchClicked() {
    QString text = searchEdit->text().trimmed();
    if (text.isEmpty()) return;

    searchButton->setEnabled(false); // 防止重复点击
    searchButton->setText("查询中...");
    translator->translate(text);
}

void MainWindow::onTranslationFinished(const QString &original, const QString &translated) {
    // 1. 显示翻译结果
    resultDisplay->setText(translated);

    // 2. 【多线程模块】：在子线程中生成随机例句（扩充至 11 个模板）
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

        QMetaObject::invokeMethod(this, [this, example](){
            exampleDisplay->setText(example);
        }, Qt::QueuedConnection);
    });

    // 3. 【数据库模块】：在主线程写入数据库
    if (DatabaseManager::instance().addHistory(original, translated)) {
        historyModel->select();
    }

    // 4. 恢复按钮状态
    searchButton->setEnabled(true);
    searchButton->setText("查询");
}
void MainWindow::onTranslationError(const QString &error) {
    resultDisplay->setText("错误: " + error);
    searchButton->setEnabled(true);
    searchButton->setText("查询");
}

void MainWindow::onHistoryItemClicked(const QModelIndex &index) {
    // 从 Model 中获取当前行的数据
    int row = index.row();
    QString original = historyModel->data(historyModel->index(row, 1)).toString();
    QString translated = historyModel->data(historyModel->index(row, 2)).toString();

    // 回显到界面
    searchEdit->setText(original);
    resultDisplay->setText(translated);

    // 更新例句展示
    QString example = QString("Example: This is a sample sentence containing '%1'.\n"
                              "例句: 这是一个包含“%2”的示例句子。")
                          .arg(original).arg(translated);
    exampleDisplay->setText(example);
}

void MainWindow::onClearClicked() {
    if (DatabaseManager::instance().clearHistory()) {
        historyModel->select();
        resultDisplay->clear();
        exampleDisplay->clear();
    }
}

void MainWindow::onFavClicked() {
    QString word = searchEdit->text().trimmed();
    QString trans = resultDisplay->toPlainText().trimmed();

    if (!word.isEmpty() && !trans.isEmpty()) {
        if (DatabaseManager::instance().addFavorite(word, trans)) {
            favoriteModel->select(); // 刷新生词本列表
        }
    }
}

void MainWindow::onFavoriteContextMenu(const QPoint &pos) {
    QModelIndex index = favoriteList->indexAt(pos);
    if (!index.isValid()) return;

    QMenu menu(this);
    QAction *deleteAction = menu.addAction("从生词本删除");

    // 获取选中的单词原文
    QString word = favoriteModel->data(favoriteModel->index(index.row(), 1)).toString();

    connect(deleteAction, &QAction::triggered, this, [this, word]() {
        if (DatabaseManager::instance().removeFavorite(word)) {
            favoriteModel->select(); // 刷新列表
        }
    });

    menu.exec(favoriteList->mapToGlobal(pos));
}

void MainWindow::onCopyClicked() {
    // 将翻译结果文本框的内容复制到系统剪贴板
    QGuiApplication::clipboard()->setText(resultDisplay->toPlainText());
}
