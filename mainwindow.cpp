#include "mainwindow.h"
#include "databasemanager.h" // 引用数据库管理类

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 1. 初始化数据库
    DatabaseManager::instance().initDatabase();

    // 2. 初始化UI
    setupUI();

    // 3. 初始化Model/View
    initModel();
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

    // 设置左右比例
    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addLayout(rightLayout, 1);
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
