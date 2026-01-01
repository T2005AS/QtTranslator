#include "translator.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QRandomGenerator>
#include <QDebug>

Translator::Translator(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

void Translator::translate(const QString &text) {
    QString salt = QString::number(QRandomGenerator::global()->generate());

    // --- 1. 最稳的 Unicode 检测中文逻辑 ---
    bool hasChinese = false;
    for (const QChar &ch : text) {
        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FA5) {
            hasChinese = true;
            break;
        }
    }
    // 如果有中文，翻译成英文(en)；如果没有中文，翻译成中文(zh)
    QString toLang = hasChinese ? "en" : "zh";

    // 调试打印：你可以在 Qt Creator 底部看到到底发了什么
    qDebug() << "Main Translate -> Input:" << text << "ToLang:" << toLang;

    // --- 2. 百度翻译请求 ---
    QString rawSign = appid + text + salt + secret;
    QByteArray sign = QCryptographicHash::hash(rawSign.toUtf8(), QCryptographicHash::Md5).toHex();

    QUrl url("http://api.fanyi.baidu.com/api/trans/vip/translate");
    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", "auto");
    query.addQueryItem("to", toLang);
    query.addQueryItem("appid", appid);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);
    url.setQuery(query);

    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, hasChinese]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            QJsonArray results = obj["trans_result"].toArray();
            if (!results.isEmpty()) {
                QString translated = results.at(0).toObject()["dst"].toString();
                QString original = results.at(0).toObject()["src"].toString();

                // 发送主翻译结果
                emit finished(original, translated);

                // --- 3. 链式获取例句 ---
                // 逻辑：如果是搜英文，直接查原文例句；如果是搜中文，查翻译出来的英文结果
                if (!hasChinese) {
                    fetchRealExamples(original);
                } else {
                    fetchRealExamples(translated);
                }
            }
        } else {
            emit errorOccurred("网络请求失败");
        }
        reply->deleteLater();
    });
}

void Translator::fetchRealExamples(const QString &word) {
    // 过滤掉可能的标点，只取第一个单词查词典
    QString cleanWord = word.split(QRegularExpression("\\s+")).at(0);
    cleanWord.remove(QRegularExpression("[^a-zA-Z]"));

    QUrl url("https://api.dictionaryapi.dev/api/v2/entries/en/" + cleanWord);
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
            if (!arr.isEmpty()) {
                QString firstEx;
                QJsonArray meanings = arr.at(0).toObject()["meanings"].toArray();
                for(int i=0; i<meanings.size(); ++i) {
                    QJsonArray defs = meanings.at(i).toObject()["definitions"].toArray();
                    for(int j=0; j<defs.size(); ++j) {
                        firstEx = defs.at(j).toObject()["example"].toString();
                        if(!firstEx.isEmpty()) break;
                    }
                    if(!firstEx.isEmpty()) break;
                }
                if(!firstEx.isEmpty()) translateExample(firstEx);
                else emit examplesReady("No real examples found.");
            }
        } else {
            emit examplesReady("Real-time examples unavailable.");
        }
        reply->deleteLater();
    });
}

void Translator::translateExample(const QString &englishEx) {
    QString salt = QString::number(QRandomGenerator::global()->generate());
    QString rawSign = appid + englishEx + salt + secret;
    QByteArray sign = QCryptographicHash::hash(rawSign.toUtf8(), QCryptographicHash::Md5).toHex();

    QUrl url("http://api.fanyi.baidu.com/api/trans/vip/translate");
    QUrlQuery query;
    query.addQueryItem("q", englishEx);
    query.addQueryItem("from", "en");
    query.addQueryItem("to", "zh");
    query.addQueryItem("appid", appid);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);
    url.setQuery(query);

    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, englishEx]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            QJsonArray results = obj["trans_result"].toArray();
            if (!results.isEmpty()) {
                QString cnEx = results.at(0).toObject()["dst"].toString();
                emit examplesReady(QString("%1\n(%2)").arg(englishEx, cnEx));
            }
        }
        reply->deleteLater();
    });
}

void Translator::onReplyFinished(QNetworkReply *reply) { Q_UNUSED(reply); }
