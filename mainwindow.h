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
#include "translator.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSearchClicked();
    void onTranslationFinished(const QString &original, const QString &translated);
    void onTranslationError(const QString &error);

private:
    void setupUI();      // 纯代码初始化UI
    Translator *translator;

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
