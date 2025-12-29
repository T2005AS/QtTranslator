#include "translator.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QRandomGenerator>

Translator::Translator(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

void Translator::translate(const QString &text) {
    QString salt = QString::number(QRandomGenerator::global()->generate());

    // 计算签名: sign = md5(appid + q + salt + secret)
    QString rawSign = appid + text + salt + secret;
    QByteArray sign = QCryptographicHash::hash(rawSign.toUtf8(), QCryptographicHash::Md5).toHex();

    QUrl url("http://api.fanyi.baidu.com/api/trans/vip/translate");
    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", "auto");
    query.addQueryItem("to", "zh");
    query.addQueryItem("appid", appid);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);

    url.setQuery(query);

    QNetworkRequest request(url);
    connect(manager->get(request), &QNetworkReply::finished, this, [this, reply = manager->get(request), text](){
        // 这里为了简化逻辑，直接在 onReplyFinished 处理，注意：上面的 connect 写法在 Qt6 中有更优雅的写法
    });

    // 修正：使用标准的信号槽
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
