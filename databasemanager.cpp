#include "databasemanager.h"
#include <QDebug>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::initDatabase() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("history.db");

    if (!db.open()) {
        qDebug() << "Error: connection with database fail" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    // 创建历史记录表：包含ID，原文，译文，时间戳
    QString sql = "CREATE TABLE IF NOT EXISTS history ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "original_text TEXT, "
                  "translated_text TEXT, "
                  "time DATETIME DEFAULT CURRENT_TIMESTAMP)";

    if (!query.exec(sql)) {
        qDebug() << "Create table error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::addHistory(const QString &original, const QString &translated) {
    QSqlQuery query;
    query.prepare("INSERT INTO history (original_text, translated_text) VALUES (?, ?)");
    query.addBindValue(original);
    query.addBindValue(translated);
    return query.exec();
}

bool DatabaseManager::clearHistory() {
    QSqlQuery query;
    return query.exec("DELETE FROM history");
}
