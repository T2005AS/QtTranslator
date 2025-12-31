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
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        if (!obj.contains("error_code")) {
            QJsonArray results = obj["trans_result"].toArray();
            if (!results.isEmpty()) {
                QString translated = results.at(0).toObject()["dst"].toString();
                QString original = results.at(0).toObject()["src"].toString();
                emit finished(original, translated);

                // --- 新增：如果是英文单词，去抓取真实例句 ---
                // 简单判断：如果不包含中文，就认为是英文单词
                if (!original.contains(QRegularExpression("[\\x4e00-\\x9fa5]"))) {
                    QUrl exUrl("https://api.dictionaryapi.dev/api/v2/entries/en/" + original);
                    QNetworkReply *exReply = manager->get(QNetworkRequest(exUrl));
                    connect(exReply, &QNetworkReply::finished, this, [this, exReply]() {
                        if (exReply->error() == QNetworkReply::NoError) {
                            QJsonDocument exDoc = QJsonDocument::fromJson(exReply->readAll());
                            QJsonArray exArr = exDoc.array();
                            QString allEx;
                            if (!exArr.isEmpty()) {
                                // 解析 JSON 获取第一个例句
                                QJsonArray meanings = exArr.at(0).toObject()["meanings"].toArray();
                                for(int i=0; i<meanings.size(); ++i) {
                                    QJsonArray defs = meanings.at(i).toObject()["definitions"].toArray();
                                    for(int j=0; j<defs.size(); ++j) {
                                        QString ex = defs.at(j).toObject()["example"].toString();
                                        if(!ex.isEmpty()) {
                                            allEx += "• " + ex + "\n";
                                            if(allEx.split("\n").size() > 3) break; // 只取前3条
                                        }
                                    }
                                }
                            }
                            if (allEx.isEmpty()) allEx = "No real-world examples found for this word.";
                            emit examplesReady(allEx);
                        }
                        exReply->deleteLater();
                    });
                } else {
                    emit examplesReady("中文查询暂不提供实时例句。");
                }
            }
        }
    }
    reply->deleteLater();
}
