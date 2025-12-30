#include "translator.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QRandomGenerator>
#include <QDebug>
#include <QRegularExpression>

Translator::Translator(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}


void Translator::translate(const QString &text) {
    QString salt = QString::number(QRandomGenerator::global()->generate());

    // --- 改进的语言检测逻辑 ---
    bool hasChinese = false;
    for (const QChar &ch : text) {
        // 中文字符的 Unicode 范围通常在 0x4E00 到 0x9FA5
        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FA5) {
            hasChinese = true;
            break;
        }
    }

    // 如果有中文，目标语言设为英文(en)，否则设为中文(zh)
    QString toLang = hasChinese ? "en" : "zh";

    // 调试信息：在 Qt Creator 底部的“应用程序输出”可以看到
    qDebug() << "输入内容:" << text << " 检测到中文:" << hasChinese << " 目标语言:" << toLang;

    // --- 签名计算 ---
    QString rawSign = appid + text + salt + secret;
    QByteArray sign = QCryptographicHash::hash(rawSign.toUtf8(), QCryptographicHash::Md5).toHex();

    // --- 构造请求 ---
    QUrl url("http://api.fanyi.baidu.com/api/trans/vip/translate");
    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", "auto");
    query.addQueryItem("to", toLang);
    query.addQueryItem("appid", appid);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);

    url.setQuery(query);
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, text]() {
        onReplyFinished(reply);
    });
}

void Translator::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
    } else {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        if (obj.contains("error_code")) {
            emit errorOccurred(obj["error_msg"].toString());
        } else {
            QJsonArray results = obj["trans_result"].toArray();
            if (!results.isEmpty()) {
                QString translated = results.at(0).toObject()["dst"].toString();
                QString original = results.at(0).toObject()["src"].toString();
                emit finished(original, translated);
            }
        }
    }
    reply->deleteLater();
}
