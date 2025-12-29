#include "mainwindow.h"
#include "databasemanager.h" // 引用数据库管理类
#include <QtConcurrent>
#include <QTimer>
#include <QRandomGenerator>
#include <QFutureWatcher>

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

    this->setWindowTitle("智能翻译字典 - Qt6.9.2");
    this->resize(800, 600);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 主布局：水平布局（左边查词，右边历史记录）
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // 左侧查询区域布局
    QVBoxLayout *leftLayout = new QVBoxLayout();

    QHBoxLayout *searchBarLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("请输入要查询的单词或句子...");
    searchButton = new QPushButton("查询");
    searchBarLayout->addWidget(searchEdit);
    searchBarLayout->addWidget(searchButton);

    leftLayout->addLayout(searchBarLayout);
    leftLayout->addWidget(new QLabel("翻译结果:"));
    resultDisplay = new QTextEdit();
    resultDisplay->setReadOnly(true);
    leftLayout->addWidget(resultDisplay);

    leftLayout->addWidget(new QLabel("例句展示:"));
    exampleDisplay = new QTextEdit();
    exampleDisplay->setReadOnly(true);
    leftLayout->addWidget(exampleDisplay);

    // 右侧历史记录区域
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(new QLabel("搜索历史:"));
    historyList = new QListView();
    rightLayout->addWidget(historyList);
    clearButton = new QPushButton("清空历史");
    clearButton->setStyleSheet("background-color: #ff3b30;"); // 红色按钮
    rightLayout->addWidget(clearButton);

    // 设置左右比例
    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addLayout(rightLayout, 1);

    this->setStyleSheet(R"(
    QMainWindow { background-color: #f5f5f7; }
    QLineEdit { padding: 8px; border: 1px solid #ccc; border-radius: 4px; background: white; }
    QPushButton { padding: 8px 20px; background-color: #007aff; color: white; border-radius: 4px; font-weight: bold; }
    QPushButton:hover { background-color: #0063cc; }
    QPushButton:disabled { background-color: #ccc; }
    QTextEdit { border: 1px solid #ddd; border-radius: 4px; background: white; font-size: 14px; }
    QListView { border: 1px solid #ddd; border-radius: 4px; background: white; }
    QLabel { font-weight: bold; color: #333; }
)");


}

void MainWindow::initModel()
{
    historyModel = new QSqlTableModel(this);
    historyModel->setTable("history");
    historyModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    historyModel->setSort(3, Qt::DescendingOrder); // 按时间倒序排列
    historyModel->select(); // 查询数据

    // 将 Model 绑定到 View (historyList)
    historyList->setModel(historyModel);

    // 设置 View 显示哪一列（显示原文列，通常是第1列，0是ID）
    historyList->setModelColumn(1);
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

    // 2. 优化例句生成逻辑：随机模板库
    QStringList templates = {
        "The word '%1' is very important in this context.\n(在这个语境下，'%2'这个词非常重要。)",
        "I don't know how to use '%1' in a sentence.\n(我不知道如何在句子中使用'%2'。)",
        "Could you please explain the meaning of '%1'?\n(你能解释一下'%2'的意思吗？)",
        "He wrote the word '%1' on the blackboard.\n(他在黑板上写下了'%2'这个词。)",
        "Is '%1' a common expression in English?\n('%2'是英语中的常用表达吗？)"
    };
    // 根据原文长度或随机数选择模板
    int index = QRandomGenerator::global()->bounded(templates.size());
    QString example = templates.at(index).arg(original, translated);
    exampleDisplay->setText(example);

    // 3. 多线程写入数据库，并使用 Watcher 监控完成状态
    auto future = QtConcurrent::run([original, translated]() {
        return DatabaseManager::instance().addHistory(original, translated);
    });

    // 创建监听器，当子线程写完后，立刻通知主线程刷新
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
        historyModel->select(); // 此时数据库一定写完了，刷新必成功
        watcher->deleteLater();
    });
    watcher->setFuture(future);

    // 4. 恢复按钮
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
