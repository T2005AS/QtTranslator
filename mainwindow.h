#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QListView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSqlTableModel>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUI();      // 纯代码初始化UI

    // UI 组件声明
    QWidget *centralWidget;
    QLineEdit *searchEdit;
    QPushButton *searchButton;
    QTextEdit *resultDisplay;
    QListView *historyList;
    QTextEdit *exampleDisplay;
    QSqlTableModel *historyModel;
    void initModel(); // 新增初始化模型的方法
};
#endif // MAINWINDOW_H
