#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class Translator : public QObject {
    Q_OBJECT
public:
    explicit Translator(QObject *parent = nullptr);
    void translate(const QString &text);

signals:
    void finished(const QString &original, const QString &translated);
    void errorOccurred(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
    const QString appid = "20251229002529367";
    const QString secret = "y3U9hBwmtRnohAS9bk33";
};

#endif // TRANSLATOR_H
