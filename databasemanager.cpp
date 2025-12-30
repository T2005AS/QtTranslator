#include "databasemanager.h"
#include <QDebug>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::initDatabase() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("history.db");

    if (!db.open()) return false;

    QSqlQuery query;
    // 表1：历史记录
    query.exec("CREATE TABLE IF NOT EXISTS history ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "original_text TEXT, "
               "translated_text TEXT, "
               "time DATETIME DEFAULT CURRENT_TIMESTAMP)");

    // 表2：生词本 (新增)
    query.exec("CREATE TABLE IF NOT EXISTS favorites ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "word TEXT UNIQUE, "
               "translation TEXT, "
               "added_time DATETIME DEFAULT CURRENT_TIMESTAMP)");
    return true;
}

bool DatabaseManager::addHistory(const QString &original, const QString &translated) {
    QSqlQuery query;
    query.prepare("INSERT INTO history (original_text, translated_text) VALUES (?, ?)");
    query.addBindValue(original);
    query.addBindValue(translated);
    return query.exec();
}

bool DatabaseManager::addFavorite(const QString &word, const QString &trans) {
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO favorites (word, translation) VALUES (?, ?)");
    query.addBindValue(word);
    query.addBindValue(trans);
    return query.exec();
}

bool DatabaseManager::clearHistory() {
    QSqlQuery query;
    return query.exec("DELETE FROM history");
}

bool DatabaseManager::removeFavorite(const QString &word) {
    QSqlQuery query;
    query.prepare("DELETE FROM favorites WHERE word = ?");
    query.addBindValue(word);
    return query.exec();
}
