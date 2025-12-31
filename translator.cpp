#include "translator.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QRandomGenerator>
#include <QRegularExpression>

Translator::Translator(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

void Translator::translate(const QString &text) {
    QString salt = QString::number(QRandomGenerator::global()->generate());

    // 检测中文逻辑
    bool hasChinese = false;
    for (const QChar &ch : text) {
        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FA5) {
            hasChinese = true;
            break;
        }
    }
    QString toLang = hasChinese ? "en" : "zh";

    // 百度翻译请求
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

    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    // 连接到具体的槽函数，解决 undefined reference 报错
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void Translator::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("网络错误: " + reply->errorString());
    } else {
        QByteArray data = reply->readAll();
        QJsonObject obj = QJsonDocument::fromJson(data).object();

        if (obj.contains("error_code")) {
            emit errorOccurred(obj["error_msg"].toString());
        } else {
            QJsonArray results = obj["trans_result"].toArray();
            if (!results.isEmpty()) {
                QString translated = results.at(0).toObject()["dst"].toString();
                QString original = results.at(0).toObject()["src"].toString();

                // 1. 发送翻译结果信号
                emit finished(original, translated);

                // 2. 智能例句逻辑：无论搜中还是英，都找其中的英文部分去查例句
                auto isChinese = [](const QString &t) {
                    for (const QChar &ch : t) {
                        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FA5) return true;
                    }
                    return false;
                };

                if (!isChinese(original)) {
                    // 输入的是英文，直接查原文例句
                    fetchRealExamples(original);
                } else if (!isChinese(translated)) {
                    // 输入的是中文，查翻译出来的英文结果的例句
                    fetchRealExamples(translated);
                } else {
                    emit examplesReady("暂无可用英文例句。");
                }
            }
        }
    }
    reply->deleteLater();
}

void Translator::fetchRealExamples(const QString &word) {
    QUrl url("https://api.dictionaryapi.dev/api/v2/entries/en/" + word);
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
            if (!arr.isEmpty()) {
                QString exText;
                QJsonArray meanings = arr.at(0).toObject()["meanings"].toArray();
                for(int i=0; i<meanings.size(); ++i) {
                    QJsonArray defs = meanings.at(i).toObject()["definitions"].toArray();
                    for(int j=0; j<defs.size(); ++j) {
                        QString ex = defs.at(j).toObject()["example"].toString();
                        if(!ex.isEmpty()) {
                            exText += "• " + ex + "\n";
                            if(exText.count('\n') >= 3) break;
                        }
                    }
                }
                emit examplesReady(exText.isEmpty() ? "No real examples found." : exText);
            }
        } else {
            emit examplesReady("Real-time examples unavailable.");
        }
        reply->deleteLater();
    });
}
