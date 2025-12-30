#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

class DatabaseManager {
public:
    static DatabaseManager& instance();
    bool initDatabase();
    bool addHistory(const QString &original, const QString &translated);
    // 老师要求的第二张表的操作：生词本收藏
    bool addFavorite(const QString &word, const QString &translation);
    bool clearHistory();
    bool removeFavorite(const QString &word);

private:
    DatabaseManager() = default;
    QSqlDatabase db;
};

#endif // DATABASEMANAGER_H
