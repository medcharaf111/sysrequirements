#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>


struct GameRequirements {
    QString cpu;
    QString gpu;
    QString ram;
    QString storage;
    
};

class GameRequirementsWorker : public QObject
{
    Q_OBJECT

public:
    GameRequirementsWorker(const QString &gameName, const QString &appId = "", QObject *parent = nullptr)
        : QObject(parent), m_gameName(gameName), m_appId(appId) { qDebug() << "GameRequirementsWorker created for:" << m_gameName << ", AppID:" << m_appId; }
    ~GameRequirementsWorker() override { qDebug() << "GameRequirementsWorker destroyed for:" << m_gameName; }

public slots:
    void processRequirementsSearch()
    {
        emit started();
        qDebug() << "GameRequirementsWorker::processRequirementsSearch started for:" << m_gameName;

        if (!m_appId.isEmpty()) {
            
            QString steamUrl = QString("https://store.steampowered.com/api/appdetails?appids=%1&l=english").arg(m_appId);
            QNetworkAccessManager* steamManager = new QNetworkAccessManager(this);
            QNetworkRequest steamRequest{QUrl(steamUrl)};
            QObject::connect(steamManager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* steamReply) {
                if (steamReply->error() == QNetworkReply::NoError) {
                    QByteArray steamResponse = steamReply->readAll();
                    QJsonDocument steamDoc = QJsonDocument::fromJson(steamResponse);
                    QJsonObject root = steamDoc.object();
                    QJsonObject appObj = root[m_appId].toObject();
                    if (appObj["success"].toBool()) {
                        QJsonObject data = appObj["data"].toObject();
                        QString gameName = data["name"].toString();
                        emit gameNameFound(gameName);
                        QJsonObject pcReqs = data["pc_requirements"].toObject();
                        QString minReq = pcReqs["minimum"].toString();
                        QString recReq = pcReqs["recommended"].toString();
                        qDebug() << "Steam minimum requirements (HTML):" << minReq;
                        qDebug() << "Steam recommended requirements (HTML):" << recReq;
                        if (!minReq.isEmpty()) {
                            
                            minReq.replace("Processor:", "\nProcessor:");
                            minReq.replace("Memory:", "\nMemory:");
                            minReq.replace("Graphics:", "\nGraphics:");
                            minReq.replace("DirectX:", "\nDirectX:");
                            minReq.replace("Storage:", "\nStorage:");
                            minReq.replace("Additional Notes:", "\nAdditional Notes:");
                            
                            GameRequirements requirements;
                            QRegularExpression cpuRe("Processor:([^\n]*)");
                            QRegularExpression ramRe("Memory:([^\n]*)");
                            QRegularExpression gpuRe("Graphics:([^\n]*)");
                            QRegularExpression storageRe("Storage:([^\n]*)");
                            QRegularExpressionMatch match;
                            match = cpuRe.match(minReq);
                            requirements.cpu = match.hasMatch() ? match.captured(1).trimmed() : "";
                            requirements.cpu.remove(QRegularExpression("<[^>]*>"));
                            match = ramRe.match(minReq);
                            requirements.ram = match.hasMatch() ? match.captured(1).trimmed() : "";
                            requirements.ram.remove(QRegularExpression("<[^>]*>"));
                            match = gpuRe.match(minReq);
                            requirements.gpu = match.hasMatch() ? match.captured(1).trimmed() : "";
                            requirements.gpu.remove(QRegularExpression("<[^>]*>"));
                            match = storageRe.match(minReq);
                            requirements.storage = match.hasMatch() ? match.captured(1).trimmed() : "";
                            requirements.storage.remove(QRegularExpression("<[^>]*>"));
                            
                            if (requirements.cpu.isEmpty() && requirements.gpu.isEmpty() && requirements.ram.isEmpty() && requirements.storage.isEmpty()) {
                                requirements.cpu = minReq;
                            }
                            emit searchFinished(requirements);
                            qDebug() << "Emitted searchFinished with Steam minimum requirements (AppID):" << requirements.cpu << requirements.gpu << requirements.ram << requirements.storage;
                            emit finished();
                            steamReply->deleteLater();
                            steamManager->deleteLater();
                            return;
                        }
                    }
                }
                
                qDebug() << "No requirements found in Steam Storefront API (AppID).";
                GameRequirements requirements;
                requirements.cpu = "No requirements found.";
                requirements.gpu = "";
                requirements.ram = "";
                requirements.storage = "";
                emit searchFinished(requirements);
                emit finished();
                steamReply->deleteLater();
                steamManager->deleteLater();
            });
            steamManager->get(steamRequest);
            return;
        }

        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QString apiKey = "df715f73748447f587032a7708b403b2";
        QString url = QString("https://api.rawg.io/api/games?key=%1&search=%2")
                        .arg(apiKey)
                        .arg(QUrl::toPercentEncoding(m_gameName));
        QNetworkRequest request{QUrl(url)};

        QObject::connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply) {
            bool foundRAWG = false;
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray response = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(response);
                QJsonObject obj = doc.object();
                QJsonArray results = obj["results"].toArray();
                for (const QJsonValue& value : results) {
                    QJsonObject game = value.toObject();
                    qDebug() << "RAWG Game:" << game["name"].toString();
                    
                    QJsonArray platforms = game["platforms"].toArray();
                    for (const QJsonValue& platVal : platforms) {
                        QJsonObject platObj = platVal.toObject();
                        QJsonObject platform = platObj["platform"].toObject();
                        qDebug() << "Platform JSON:" << QJsonDocument(platObj).toJson(QJsonDocument::Compact);
                        if (platform["name"].toString().toLower() == "pc") {
                            QJsonObject reqs = platObj["requirements"].toObject();
                            qDebug() << "PC requirements JSON:" << QJsonDocument(reqs).toJson(QJsonDocument::Compact);
                            QString minReq = reqs["minimum"].toString();
                            QString recReq = reqs["recommended"].toString();
                            if (!minReq.isEmpty()) {
                                GameRequirements requirements;
                                requirements.cpu = minReq; 
                                requirements.gpu = "";
                                requirements.ram = "";
                                requirements.storage = "";
                                emit searchFinished(requirements);
                                qDebug() << "Emitted searchFinished with minimum requirements:" << minReq;
                                emit finished();
                                reply->deleteLater();
                                manager->deleteLater();
                                return;
                            }
                        }
                    }
                }
            }
            
            qDebug() << "No PC requirements found in RAWG response. Trying Steam Storefront API...";
            
            
            QMap<QString, QString> appIdMap = {
                {"cyberpunk 2077", "1091500"},
                {"half-life 2", "220"},
                {"elden ring", "1245620"}
            };
            QString key = m_gameName.trimmed().toLower();
            QString appid = appIdMap.value(key, "");
            if (!appid.isEmpty()) {
                QString steamUrl = QString("https://store.steampowered.com/api/appdetails?appids=%1&l=english").arg(appid);
                QNetworkAccessManager* steamManager = new QNetworkAccessManager(this);
                QNetworkRequest steamRequest{QUrl(steamUrl)};
                QObject::connect(steamManager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* steamReply) {
                    if (steamReply->error() == QNetworkReply::NoError) {
                        QByteArray steamResponse = steamReply->readAll();
                        QJsonDocument steamDoc = QJsonDocument::fromJson(steamResponse);
                        QJsonObject root = steamDoc.object();
                        QJsonObject appObj = root[appid].toObject();
                        if (appObj["success"].toBool()) {
                            QJsonObject data = appObj["data"].toObject();
                            QString gameName = data["name"].toString();
                            emit gameNameFound(gameName);
                            QJsonObject pcReqs = data["pc_requirements"].toObject();
                            QString minReq = pcReqs["minimum"].toString();
                            QString recReq = pcReqs["recommended"].toString();
                            qDebug() << "Steam minimum requirements (HTML):" << minReq;
                            qDebug() << "Steam recommended requirements (HTML):" << recReq;
                            if (!minReq.isEmpty()) {
                                
                                minReq.replace("Processor:", "\nProcessor:");
                                minReq.replace("Memory:", "\nMemory:");
                                minReq.replace("Graphics:", "\nGraphics:");
                                minReq.replace("DirectX:", "\nDirectX:");
                                minReq.replace("Storage:", "\nStorage:");
                                minReq.replace("Additional Notes:", "\nAdditional Notes:");
                                
                                GameRequirements requirements;
                                QRegularExpression cpuRe("Processor:([^\n]*)");
                                QRegularExpression ramRe("Memory:([^\n]*)");
                                QRegularExpression gpuRe("Graphics:([^\n]*)");
                                QRegularExpression storageRe("Storage:([^\n]*)");
                                QRegularExpressionMatch match;
                                match = cpuRe.match(minReq);
                                requirements.cpu = match.hasMatch() ? match.captured(1).trimmed() : "";
                                requirements.cpu.remove(QRegularExpression("<[^>]*>"));
                                match = ramRe.match(minReq);
                                requirements.ram = match.hasMatch() ? match.captured(1).trimmed() : "";
                                requirements.ram.remove(QRegularExpression("<[^>]*>"));
                                match = gpuRe.match(minReq);
                                requirements.gpu = match.hasMatch() ? match.captured(1).trimmed() : "";
                                requirements.gpu.remove(QRegularExpression("<[^>]*>"));
                                match = storageRe.match(minReq);
                                requirements.storage = match.hasMatch() ? match.captured(1).trimmed() : "";
                                requirements.storage.remove(QRegularExpression("<[^>]*>"));
                                
                                if (requirements.cpu.isEmpty() && requirements.gpu.isEmpty() && requirements.ram.isEmpty() && requirements.storage.isEmpty()) {
                                    requirements.cpu = minReq;
                                }
                                emit searchFinished(requirements);
                                qDebug() << "Emitted searchFinished with Steam minimum requirements:" << requirements.cpu << requirements.gpu << requirements.ram << requirements.storage;
                                emit finished();
                                steamReply->deleteLater();
                                steamManager->deleteLater();
                                reply->deleteLater();
                                manager->deleteLater();
                                return;
                            }
                        }
                    }
                    
                    qDebug() << "No requirements found in Steam Storefront API.";
                    GameRequirements requirements;
                    requirements.cpu = "No requirements found.";
                    requirements.gpu = "";
                    requirements.ram = "";
                    requirements.storage = "";
                    emit searchFinished(requirements);
                    emit finished();
                    steamReply->deleteLater();
                    steamManager->deleteLater();
                    reply->deleteLater();
                    manager->deleteLater();
                });
                steamManager->get(steamRequest);
                return;
            }
            
            GameRequirements requirements;
            requirements.cpu = "No requirements found.";
            requirements.gpu = "";
            requirements.ram = "";
            requirements.storage = "";
            emit searchFinished(requirements);
            emit finished();
            reply->deleteLater();
            manager->deleteLater();
        });

        manager->get(request);
    }

signals:
    void started();
    void finished();
    void error(const QString &message);
    void searchFinished(const GameRequirements &requirements);
    void gameNameFound(const QString &name);

private:
    QString m_gameName;
    QString m_appId;
}; 