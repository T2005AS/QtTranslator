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
#include <QTabWidget>
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
    void onHistoryItemClicked(const QModelIndex &index);
    void onClearClicked();
    void onFavClicked();
    void onFavoriteContextMenu(const QPoint &pos);
    void onCopyClicked();
    void onAboutClicked(); // 第19次新增

private:
    void setupUI();
    void initModel();

    QWidget *centralWidget;
    QLineEdit *searchEdit;
    QPushButton *searchButton;
    QPushButton *favButton;
    QPushButton *copyButton;
    QPushButton *resetButton; // 第18次新增
    QPushButton *aboutButton; // 第19次新增
    QPushButton *clearButton;

    QTextEdit *resultDisplay;
    QTextEdit *exampleDisplay;

    QTabWidget *tabWidget;
    QListView *historyList;
    QListView *favoriteList;

    QSqlTableModel *historyModel;
    QSqlTableModel *favoriteModel;
    Translator *translator;
};
#endif // MAINWINDOW_H
