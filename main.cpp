#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <thread>
#include <chrono>
#include <fstream>
#include <string>
#include "dxtextmake.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QHeaderView>
#include <QDateTime>
#include <QStyle>
#include <QThread>
#include "DxDiagWorker.h"
#include <QDebug>
#include <QLineEdit>
#include <QHBoxLayout>
#include "GameRequirementsWorker.h"
#include <QRegularExpression>
#include <QMap>
#include <QIcon>


int parseRam(const QString& ramStr) {
    QRegularExpression re("(\\d+)(\\s*)(MB|GB|TB)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(ramStr);
    if (match.hasMatch()) {
        int value = match.captured(1).toInt();
        QString unit = match.captured(3).toUpper();
        if (unit == "GB") return value * 1024;
        if (unit == "TB") return value * 1024 * 1024;
        return value;
    }
    return 0;
}

int parseStorage(const QString& storageStr) {
    QRegularExpression re("(\\d+)(\\s*)(MB|GB|TB)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(storageStr);
    if (match.hasMatch()) {
        int value = match.captured(1).toInt();
        QString unit = match.captured(3).toUpper();
        if (unit == "GB") return value * 1024;
        if (unit == "TB") return value * 1024 * 1024;
        return value;
    }
    return 0;
}

int cpuRank(const QString& cpuStr) {
    
    static QMap<QString, int> cpuRanks = {
        {"i3", 1}, {"i5", 2}, {"i7", 3}, {"i9", 4},
        {"ryzen 3", 1}, {"ryzen 5", 2}, {"ryzen 7", 3}, {"ryzen 9", 4}
    };
    QString s = cpuStr.toLower();
    for (auto it = cpuRanks.begin(); it != cpuRanks.end(); ++it) {
        if (s.contains(it.key())) return it.value();
    }
    return 0;
}

int gpuRank(const QString& gpuStr) {
    QString s = gpuStr.toLower();
    static QMap<QString, int> seriesBase = {
        {"rtx", 1000}, {"gtx", 800}, {"gt", 600},
        {"rx", 900}, {"r9", 700}, {"r7", 600}, {"r5", 500},
        {"arc", 850},
        {"quadro", 700}, {"tesla", 700},
        {"hd", 200}, {"iris", 300}, {"vega", 400},
        {"mx", 250},
        {"uhd", 100}, {"intel hd", 100}, {"intel iris", 200}
    };
    int bestRank = 0;
    for (auto it = seriesBase.begin(); it != seriesBase.end(); ++it) {
        if (s.contains(it.key())) {
            
            QRegularExpression numRe("(\\d{3,4})");
            QRegularExpressionMatch numMatch = numRe.match(s);
            int modelNum = numMatch.hasMatch() ? numMatch.captured(1).toInt() : 0;
            int rank = it.value() + modelNum;
            if (rank > bestRank) bestRank = rank;
        }
    }
    
    static QMap<QString, int> gpuRanks = {
        {"gtx 750", 751}, {"gtx 950", 951}, {"gtx 960", 960}, {"gtx 970", 970}, {"gtx 1050", 1050}, {"gtx 1060", 1060}, {"gtx 1070", 1070}, {"gtx 1080", 1080},
        {"gtx 1650", 1650}, {"gtx 1660", 1660}, {"rtx 2060", 2060}, {"rtx 2070", 2070}, {"rtx 2080", 2080}, {"rtx 3050", 3050}, {"rtx 3060", 3060}, {"rtx 3070", 3070}, {"rtx 3080", 3080}, {"rtx 4060", 4060}, {"rtx 4070", 4070}, {"rtx 4080", 4080},
        {"rx 560", 560}, {"rx 570", 570}, {"rx 580", 580}, {"rx 590", 590}, {"rx 5500", 5500}, {"rx 5600", 5600}, {"rx 5700", 5700}, {"rx 6600", 6600}, {"rx 6700", 6700}, {"rx 6800", 6800}, {"rx 6900", 6900},
        {"arc a380", 1380}, {"arc a750", 1750}, {"arc a770", 1770},
        {"quadro p2000", 2200}, {"quadro rtx 4000", 4000},
        {"mx150", 1150}, {"mx250", 1250}, {"mx330", 1330},
        {"intel hd", 100}, {"intel iris", 200}, {"uhd", 100}
    };
    for (auto it = gpuRanks.begin(); it != gpuRanks.end(); ++it) {
        if (s.contains(it.key())) {
            if (it.value() > bestRank) bestRank = it.value();
        }
    }
    return bestRank;
}

int parseVram(const QString& str) {
    QRegularExpression re("(\\d+)(\\s*)(MB|GB)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(str);
    if (match.hasMatch()) {
        int value = match.captured(1).toInt();
        QString unit = match.captured(3).toUpper();
        if (unit == "GB") return value * 1024;
        return value;
    }
    return 0;
}

class DxDiagWidget : public QWidget {
    Q_OBJECT

public:
    DxDiagWidget(QWidget *parent = nullptr) : QWidget(parent) {
        qDebug() << "DxDiagWidget created";
        auto *mainLayout = new QVBoxLayout(this);

        
        auto *dxdiagLabel = new QLabel("DxDiag Output:", this);
        mainLayout->addWidget(dxdiagLabel);

        treeWidget = new QTreeWidget(this);
        treeWidget->setHeaderLabels({"Item", "Details"});
        treeWidget->header()->setStretchLastSection(false);
        treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        mainLayout->addWidget(treeWidget);

        auto *generateDxDiagButton = new QPushButton("Generate/Refresh DxDiag", this);
        mainLayout->addWidget(generateDxDiagButton);

        
        auto *gameRequirementsLabel = new QLabel("Game Requirements:", this);
        mainLayout->addWidget(gameRequirementsLabel);

        auto *gameInputLayout = new QHBoxLayout();
        gameNameLineEdit = new QLineEdit(this);
        gameNameLineEdit->setPlaceholderText("Enter game name");
        gameInputLayout->addWidget(gameNameLineEdit);

        
        appIdLineEdit = new QLineEdit(this);
        appIdLineEdit->setPlaceholderText("Enter Steam AppID (optional)");
        gameInputLayout->addWidget(appIdLineEdit);

        auto *searchRequirementsButton = new QPushButton("Search Requirements", this);
        gameInputLayout->addWidget(searchRequirementsButton);

        mainLayout->addLayout(gameInputLayout);

    
        auto *comparisonLabel = new QLabel("Comparison Results:", this);
        mainLayout->addWidget(comparisonLabel);

        comparisonTreeWidget = new QTreeWidget(this);
        comparisonTreeWidget->setHeaderLabels({"Requirement", "Status", "Your System", "Minimum Required"});
        comparisonTreeWidget->header()->setStretchLastSection(false);
        comparisonTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        comparisonTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        comparisonTreeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        comparisonTreeWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        mainLayout->addWidget(comparisonTreeWidget);

        
        statusLabel = new QLabel("Ready.", this);
        mainLayout->addWidget(statusLabel);

        mainLayout->addStretch(); 

        setLayout(mainLayout);
        setWindowTitle("projet game requirements");
        setWindowIcon(QIcon("icon.ico"));
        resize(800, 800); 

        
        this->setStyleSheet(R"(
            QWidget {
                background-color: #333;
                color: #ccc;
                font-family: "Segoe UI", "Helvetica Neue", sans-serif;
            }
            QTreeWidget {
                background-color: #444;
                color: #ccc;
                border: 1px solid #555;
                alternate-background-color: #4a4a4a; 
            }
            QTreeWidget::item {
                padding: 2px;
            }
            QTreeWidget::item:selected {
                background-color: #5a5a5a;
            }
            QPushButton {
                background-color: #555;
                color: #fff;
                border: none;
                padding: 10px 20px;
                text-align: center;
                text-decoration: none;
                font-size: 14px;
                margin: 4px 2px;
                border-radius: 5px;
            }
            QPushButton:hover {
                background-color: #777;
            }
            QLabel {
                color: #aaa;
                margin-top: 5px;
            }
             QLineEdit {
                background-color: #555;
                color: #fff;
                border: 1px solid #777;
                padding: 5px;
             }
             QLineEdit:focus {
                 border: 1px solid #0078d7; 
             }
        )");

    
        worker = new DxDiagWorker();
        worker->moveToThread(&workerThread);

       
        connect(&workerThread, &QThread::started, worker, &DxDiagWorker::processDxDiag);
        connect(worker, &DxDiagWorker::finished, worker, &DxDiagWorker::deleteLater);
        connect(worker, &DxDiagWorker::finished, &workerThread, &QThread::quit, Qt::QueuedConnection);
        connect(&workerThread, &QThread::finished, this, &DxDiagWidget::onWorkerThreadFinished, Qt::QueuedConnection);

        connect(generateDxDiagButton, &QPushButton::clicked, this, &DxDiagWidget::onGenerateClicked);
        connect(worker, &DxDiagWorker::started, this, &DxDiagWidget::onWorkerStarted, Qt::QueuedConnection);
        connect(worker, &DxDiagWorker::finished, this, &DxDiagWidget::onWorkerFinished, Qt::QueuedConnection);
        connect(worker, &DxDiagWorker::error, this, &DxDiagWidget::onWorkerError, Qt::QueuedConnection);
        connect(worker, &DxDiagWorker::parsingFinished, this, &DxDiagWidget::onParsingFinished, Qt::QueuedConnection);

        connect(searchRequirementsButton, &QPushButton::clicked, this, &DxDiagWidget::onSearchRequirementsClicked);

        
        gameSearchWorker = new GameRequirementsWorker(""); 
        gameSearchWorker->moveToThread(&gameSearchThread);

        
        connect(&gameSearchThread, &QThread::started, gameSearchWorker, &GameRequirementsWorker::processRequirementsSearch);
        connect(gameSearchWorker, &GameRequirementsWorker::finished, gameSearchWorker, &GameRequirementsWorker::deleteLater);
        connect(gameSearchWorker, &GameRequirementsWorker::finished, &gameSearchThread, &QThread::quit, Qt::QueuedConnection);
        connect(&gameSearchThread, &QThread::finished, this, &DxDiagWidget::onGameSearchThreadFinished, Qt::QueuedConnection);

        connect(gameSearchWorker, &GameRequirementsWorker::started, this, &DxDiagWidget::onGameSearchStarted, Qt::QueuedConnection);
        connect(gameSearchWorker, &GameRequirementsWorker::finished, this, &DxDiagWidget::onGameSearchFinished, Qt::QueuedConnection);
        connect(gameSearchWorker, &GameRequirementsWorker::error, this, &DxDiagWidget::onGameSearchError, Qt::QueuedConnection);
        connect(gameSearchWorker, &GameRequirementsWorker::searchFinished, this, &DxDiagWidget::onGameSearchFinishedWithResults, Qt::QueuedConnection);
        connect(gameSearchWorker, &GameRequirementsWorker::gameNameFound, this, &DxDiagWidget::onGameNameFound, Qt::QueuedConnection);
    }

    ~DxDiagWidget() override {
        qDebug() << "DxDiagWidget destroyed";
        
        if (workerThread.isRunning()) {
            workerThread.quit();
            workerThread.wait(2000); 
            if (workerThread.isRunning()) {
                qDebug() << "DxDiag worker thread did not quit, terminating";
                workerThread.terminate(); 
                workerThread.wait();
            }
        }
        if (gameSearchThread.isRunning()) {
            gameSearchThread.quit();
            gameSearchThread.wait(2000); 
            if (gameSearchThread.isRunning()) {
                qDebug() << "Game search worker thread did not quit, terminating";
                gameSearchThread.terminate(); 
                gameSearchThread.wait();
            }
        }

        qDebug() << "Worker threads joined and widget destroyed";
    }

private slots:
    void onGenerateClicked() {
        qDebug() << "onGenerateClicked";
   
        if (!workerThread.isRunning()) {
             treeWidget->clear(); 
            
             if (!worker) {
                 worker = new DxDiagWorker();
                 worker->moveToThread(&workerThread);
                 
                 connect(&workerThread, &QThread::started, worker, &DxDiagWorker::processDxDiag);
                 connect(worker, &DxDiagWorker::finished, worker, &DxDiagWorker::deleteLater);
                 connect(worker, &DxDiagWorker::finished, &workerThread, &QThread::quit, Qt::QueuedConnection);
                 connect(worker, &DxDiagWorker::started, this, &DxDiagWidget::onWorkerStarted, Qt::QueuedConnection);
                 connect(worker, &DxDiagWorker::finished, this, &DxDiagWidget::onWorkerFinished, Qt::QueuedConnection);
                 connect(worker, &DxDiagWorker::error, this, &DxDiagWidget::onWorkerError, Qt::QueuedConnection);
                 connect(worker, &DxDiagWorker::parsingFinished, this, &DxDiagWidget::onParsingFinished, Qt::QueuedConnection);
             }
             workerThread.start();
             qDebug() << "Worker thread started";
        } else {
            qDebug() << "Worker thread is already running";
        }
    }

    void onWorkerStarted() {
        qDebug() << "onWorkerStarted";
         statusLabel->setText("Generating dxdiag report...");
    }

    void onWorkerFinished() {
        qDebug() << "onWorkerFinished";
       
    }

    void onWorkerError(const QString &message) {
        qDebug() << "onWorkerError:" << message;
         statusLabel->setText("Error: " + message);
         QMessageBox::warning(this, "Error", message);
    }

    void onParsingFinished(const QList<DxDiagSectionData> &sectionsData) {
        qDebug() << "onParsingFinished with" << sectionsData.size() << "sections. Widget instance:" << this;
    
        m_dxdiagData.clear(); 
        for (const auto& section : sectionsData) {
            DxDiagSectionData newSection;
            newSection.sectionName = section.sectionName;
          
            for (const auto& item : section.items) {
                QStringList newItem;
                for (const auto& value : item) {
                    newItem.append(value);
                }
                newSection.items.append(newItem);
            }
            m_dxdiagData.append(newSection);
        }

        qDebug() << "Copied" << m_dxdiagData.size() << "sections to m_dxdiagData.";

      
        m_systemSpecs.clear(); 
        qDebug() << "Extracting system specs from m_dxdiagData...";

        const DxDiagSectionData* systemInfoSection = nullptr;
        const DxDiagSectionData* displayDevicesSection = nullptr;

        for (const auto& section : m_dxdiagData) {
            if (section.sectionName == "SystemInformation") {
                systemInfoSection = &section;
            } else if (section.sectionName == "DisplayDevices") {
                displayDevicesSection = &section;
            }
        }

        if (systemInfoSection) {
            qDebug() << "System Information section found for extraction.";
            for (const auto& item : systemInfoSection->items) {
                if (item.size() > 1) { 
                    QString key = item.first();
                    QString value = item.last();
                    if (key == "Processor") {
                        m_systemSpecs["CPU"] = value;
                        qDebug() << "Extracted and mapped CPU:" << value;
                    } else if (key == "Memory") {
                        m_systemSpecs["RAM"] = value;
                        qDebug() << "Extracted and mapped RAM:" << value;
                    }
                }
            }
        }

        if (displayDevicesSection) {
             qDebug() << "Display Devices section found for extraction.";
            for (const auto& item : displayDevicesSection->items) {
                 if (item.size() > 1) { 
                    QString key = item.first();
                    QString value = item.last();
                    if (key == "CardName") { 
                        m_systemSpecs["GPU"] = value;
                        qDebug() << "Extracted and mapped GPU:" << value;
                    }
                 }
            }
        }

       
        const DxDiagSectionData* logicalDisksSection = nullptr;
        for (const auto& section : m_dxdiagData) {
            if (section.sectionName == "LogicalDisks") {
                logicalDisksSection = &section;
                break;
            }
        }

        if (logicalDisksSection) {
            qDebug() << "LogicalDisks section found for extraction.";
            QStringList storageDetails;
            double maxFreeGb = 0.0;
            for (const auto& item : logicalDisksSection->items) {
                
                 if (item.size() >= 3) {
                     
                     QRegularExpression sizeRe("(\\d{10,})");
                     QString sizeStr = item.at(2);
                     QString freeStr = item.at(1);
                     QRegularExpressionMatch sizeMatch = sizeRe.match(sizeStr);
                     QRegularExpressionMatch freeMatch = sizeRe.match(freeStr);
                     QString sizeGb = sizeStr, freeGb = freeStr;
                     double freeGbVal = 0.0;
                     if (sizeMatch.hasMatch()) {
                         double gb = sizeMatch.captured(1).toLongLong() / 1073741824.0;
                         sizeGb = QString::number(gb, 'f', 0) + " GB";
                     }
                     if (freeMatch.hasMatch()) {
                         double gb = freeMatch.captured(1).toLongLong() / 1073741824.0;
                         freeGb = QString::number(gb, 'f', 0) + " GB";
                         freeGbVal = gb;
                     }
                     if (freeGbVal > maxFreeGb) maxFreeGb = freeGbVal;
                     storageDetails.append(item.at(0) + " (" + sizeGb + ") Free: " + freeGb);
                 }
            }
            if (!storageDetails.isEmpty()) {
                m_systemSpecs["Storage"] = QString::number(maxFreeGb, 'f', 0) + " GB";
                m_systemSpecs["StorageDisplay"] = storageDetails.join("; ");
                qDebug() << "Extracted and mapped Storage:" << m_systemSpecs["Storage"];
            }
        }

        qDebug() << "Finished extracting system specs. m_systemSpecs content:";
         for(auto it = m_systemSpecs.begin(); it != m_systemSpecs.end(); ++it) {
            qDebug() << it.key() << ":" << it.value();
        }

       
        for (const auto &sectionData : sectionsData) {
            QTreeWidgetItem* sectionItem = new QTreeWidgetItem(treeWidget, {sectionData.sectionName});
            
            if (sectionData.sectionName == "LogicalDisks") {
                
                for (const auto &item : sectionData.items) {
                    
                    if (item.size() >= 3) {
                         new QTreeWidgetItem(sectionItem, {item.at(0), item.at(1) + ", " + item.at(2)}); 
                         new QTreeWidgetItem(sectionItem, {item.at(0)}); 
                    }
                }
            } else {
                
                for (const auto &item : sectionData.items) {
                    if (item.size() == 2) {
                         new QTreeWidgetItem(sectionItem, {item.at(0), item.at(1)});
                    } else if (item.size() == 1) {
                         new QTreeWidgetItem(sectionItem, {item.at(0)}); 
                    }
                }
            }

            
            if (sectionItem->childCount() > 0) {
                sectionItem->setText(0, "🟢 " + sectionItem->text(0));
            } else {
                sectionItem->setText(0, "🔴 " + sectionItem->text(0));
            }
        }

       
        for(int i = 0; i < qMin(treeWidget->topLevelItemCount(), 5); ++i) {
            treeWidget->topLevelItem(i)->setExpanded(true);
        }

       
        treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

        statusLabel->setText("DxDiag report generated on " + QDateTime::currentDateTime().toString() + ".");
        qDebug() << "UI updated and status label set";

        
        if (!m_gameRequirements.cpu.isEmpty()) { 
            performComparison();
        }
    }

    void onWorkerThreadFinished() {
        qDebug() << "onWorkerThreadFinished";
        
        if (worker) {
             worker = nullptr; 
        }
    }

    void onSearchRequirementsClicked() {
        qDebug() << "onSearchRequirementsClicked";
        QString gameName = gameNameLineEdit->text().trimmed();
        QString appId = appIdLineEdit->text().trimmed();
        if (gameName.isEmpty() && appId.isEmpty()) {
            QMessageBox::warning(this, "Input Error", "Please enter a game name or Steam AppID.");
            return;
        }

        if (!gameSearchThread.isRunning()) {
            
            if (!gameSearchWorker) {
                 gameSearchWorker = new GameRequirementsWorker(gameName, appId);
                 gameSearchWorker->moveToThread(&gameSearchThread);
                
                 connect(&gameSearchThread, &QThread::started, gameSearchWorker, &GameRequirementsWorker::processRequirementsSearch);
                 connect(gameSearchWorker, &GameRequirementsWorker::finished, gameSearchWorker, &GameRequirementsWorker::deleteLater);
                 connect(gameSearchWorker, &GameRequirementsWorker::finished, &gameSearchThread, &QThread::quit, Qt::QueuedConnection);
                 connect(gameSearchWorker, &GameRequirementsWorker::started, this, &DxDiagWidget::onGameSearchStarted, Qt::QueuedConnection);
                 connect(gameSearchWorker, &GameRequirementsWorker::finished, this, &DxDiagWidget::onGameSearchFinished, Qt::QueuedConnection);
                 connect(gameSearchWorker, &GameRequirementsWorker::error, this, &DxDiagWidget::onGameSearchError, Qt::QueuedConnection);
                 connect(gameSearchWorker, &GameRequirementsWorker::searchFinished, this, &DxDiagWidget::onGameSearchFinishedWithResults, Qt::QueuedConnection);
                 connect(gameSearchWorker, &GameRequirementsWorker::gameNameFound, this, &DxDiagWidget::onGameNameFound, Qt::QueuedConnection);
            } else {
                 
                 gameSearchWorker->deleteLater();
                 gameSearchThread.wait(1000); 
                 gameSearchWorker = new GameRequirementsWorker(gameName, appId);
                 gameSearchWorker->moveToThread(&gameSearchThread);
                 
                  connect(&gameSearchThread, &QThread::started, gameSearchWorker, &GameRequirementsWorker::processRequirementsSearch);
                  connect(gameSearchWorker, &GameRequirementsWorker::finished, gameSearchWorker, &GameRequirementsWorker::deleteLater);
                  connect(gameSearchWorker, &GameRequirementsWorker::finished, &gameSearchThread, &QThread::quit, Qt::QueuedConnection);
                  connect(gameSearchWorker, &GameRequirementsWorker::started, this, &DxDiagWidget::onGameSearchStarted, Qt::QueuedConnection);
                  connect(gameSearchWorker, &GameRequirementsWorker::finished, this, &DxDiagWidget::onGameSearchFinished, Qt::QueuedConnection);
                  connect(gameSearchWorker, &GameRequirementsWorker::error, this, &DxDiagWidget::onGameSearchError, Qt::QueuedConnection);
                  connect(gameSearchWorker, &GameRequirementsWorker::searchFinished, this, &DxDiagWidget::onGameSearchFinishedWithResults, Qt::QueuedConnection);
                  connect(gameSearchWorker, &GameRequirementsWorker::gameNameFound, this, &DxDiagWidget::onGameNameFound, Qt::QueuedConnection);
            }

            statusLabel->setText("Searching for requirements for: " + gameName + "...");
            gameSearchThread.start();
            qDebug() << "Game search thread started for:" << gameName;
        } else {
            qDebug() << "Game search thread is already running";
             statusLabel->setText("Search in progress...");
        }
    }

    void onGameSearchStarted() {
        qDebug() << "onGameSearchStarted";
         
    }

    void onGameSearchFinished() {
        qDebug() << "onGameSearchFinished";
       
    }

    void onGameSearchError(const QString &message) {
        qDebug() << "onGameSearchError:" << message;
         statusLabel->setText("Search Error: " + message);
         QMessageBox::warning(this, "Search Error", message);
    }

    void onGameSearchFinishedWithResults(const GameRequirements &requirements) {
        qDebug() << "onGameSearchFinishedWithResults";
        m_gameRequirements = requirements; 

        QString label = "Requirements found for " + gameNameLineEdit->text().trimmed() + ". Ready to compare.";
        if (!m_lastGameName.isEmpty()) {
            label = "Steam Game: " + m_lastGameName + "\n" + label;
        }
        statusLabel->setText(label);

       
        if (!m_dxdiagData.isEmpty()) {
            performComparison();
        } else {
             statusLabel->setText("Requirements found for " + gameNameLineEdit->text().trimmed() + ". Please generate DxDiag report for comparison.");
        }
    }

     void onGameSearchThreadFinished() {
         qDebug() << "onGameSearchThreadFinished";
         if (gameSearchWorker) {
              gameSearchWorker = nullptr;
         }
     }

    void onGameNameFound(const QString& name) {
        if (!name.isEmpty()) {
            m_lastGameName = name;
            statusLabel->setText("Steam Game: " + name);
        }
    }

   
    void performComparison() {
        qDebug() << "Performing comparison. Widget instance:" << this;
        comparisonTreeWidget->clear(); 
        qDebug() << "m_dxdiagData size in performComparison:" << m_dxdiagData.size();

        
        qDebug() << "--- START performComparison DEBUG ---";
        qDebug() << "Checking for keys in m_systemSpecs map:";
        qDebug() << "Contains 'CPU':" << m_systemSpecs.contains("CPU");
        qDebug() << "Contains 'GPU':" << m_systemSpecs.contains("GPU");
        qDebug() << "Contains 'RAM':" << m_systemSpecs.contains("RAM");
        qDebug() << "Contains 'Storage':" << m_systemSpecs.contains("Storage"); 

      
        QString cpuStatus = "Unknown";
        if (m_systemSpecs.contains("CPU") && !m_gameRequirements.cpu.isEmpty()) {
            int userRank = cpuRank(m_systemSpecs["CPU"]);
            int reqRank = cpuRank(m_gameRequirements.cpu);
            if (userRank > 0 && reqRank > 0) {
                cpuStatus = (userRank >= reqRank) ? "Meets or Exceeds" : "May Not Meet";
            } else {
                cpuStatus = "Unknown";
            }
        } else if (!m_gameRequirements.cpu.isEmpty() && !m_systemSpecs.contains("CPU")){
            cpuStatus = "System CPU Info Not Found";
        } else if (m_gameRequirements.cpu.isEmpty() && m_systemSpecs.contains("CPU")){
            cpuStatus = "Requirement Not Specified";
        }
        QTreeWidgetItem* cpuItem = new QTreeWidgetItem(comparisonTreeWidget, {"CPU", cpuStatus, m_systemSpecs.value("CPU", "N/A"), m_gameRequirements.cpu});
        cpuItem->setForeground(1, (cpuStatus == "Meets or Exceeds") ? QBrush(Qt::green) : (cpuStatus == "May Not Meet" ? QBrush(Qt::red) : QBrush(Qt::yellow)));

       
        QString gpuStatus = "Unknown";
        if (m_systemSpecs.contains("GPU") && !m_gameRequirements.gpu.isEmpty()) {
            int userRank = gpuRank(m_systemSpecs["GPU"]);
            int reqRank = gpuRank(m_gameRequirements.gpu);
            if (userRank > 0 && reqRank > 0) {
                if (userRank > reqRank) {
                    gpuStatus = "Meets or Exceeds";
                } else if (userRank == reqRank) {
                    
                    int userVram = parseVram(m_systemSpecs["GPU"]);
                    int reqVram = parseVram(m_gameRequirements.gpu);
                    if (userVram > 0 && reqVram > 0) {
                        gpuStatus = (userVram >= reqVram) ? "Meets or Exceeds" : "May Not Meet";
                    } else {
                        gpuStatus = "Meets or Exceeds"; 
                    }
                } else {
                    gpuStatus = "May Not Meet";
                }
            } else {
               
                int userVram = parseVram(m_systemSpecs["GPU"]);
                int reqVram = parseVram(m_gameRequirements.gpu);
                if (userVram > 0 && reqVram > 0) {
                    gpuStatus = (userVram >= reqVram) ? "Meets or Exceeds" : "May Not Meet";
                } else {
                    gpuStatus = "Unknown";
                }
            }
        } else if (!m_gameRequirements.gpu.isEmpty() && !m_systemSpecs.contains("GPU")){
            gpuStatus = "System GPU Info Not Found";
        } else if (m_gameRequirements.gpu.isEmpty() && m_systemSpecs.contains("GPU")){
            gpuStatus = "Requirement Not Specified";
        }
        QTreeWidgetItem* gpuItem = new QTreeWidgetItem(comparisonTreeWidget, {"GPU", gpuStatus, m_systemSpecs.value("GPU", "N/A"), m_gameRequirements.gpu});
        gpuItem->setForeground(1, (gpuStatus == "Meets or Exceeds") ? QBrush(Qt::green) : (gpuStatus == "May Not Meet" ? QBrush(Qt::red) : QBrush(Qt::yellow)));

        
        QString ramStatus = "Unknown";
        if (m_systemSpecs.contains("RAM") && !m_gameRequirements.ram.isEmpty()) {
            int userRam = parseRam(m_systemSpecs["RAM"]);
            int reqRam = parseRam(m_gameRequirements.ram);
            if (userRam > 0 && reqRam > 0) {
                ramStatus = (userRam >= reqRam) ? "Meets or Exceeds" : "May Not Meet";
            } else {
                ramStatus = "Unknown";
            }
        } else if (!m_gameRequirements.ram.isEmpty() && !m_systemSpecs.contains("RAM")){
            ramStatus = "System RAM Info Not Found";
        } else if (m_gameRequirements.ram.isEmpty() && m_systemSpecs.contains("RAM")){
            ramStatus = "Requirement Not Specified";
        }
        QTreeWidgetItem* ramItem = new QTreeWidgetItem(comparisonTreeWidget, {"RAM", ramStatus, m_systemSpecs.value("RAM", "N/A"), m_gameRequirements.ram});
        ramItem->setForeground(1, (ramStatus == "Meets or Exceeds") ? QBrush(Qt::green) : (ramStatus == "May Not Meet" ? QBrush(Qt::red) : QBrush(Qt::yellow)));

        
        QString storageStatus = "Unknown";
        if (m_systemSpecs.contains("Storage") && !m_gameRequirements.storage.isEmpty()) {
            int userStorage = parseStorage(m_systemSpecs["Storage"]);
            int reqStorage = parseStorage(m_gameRequirements.storage);
            if (userStorage > 0 && reqStorage > 0) {
                storageStatus = (userStorage >= reqStorage) ? "Meets or Exceeds" : "May Not Meet";
            } else {
                storageStatus = "Unknown";
            }
        } else if (!m_gameRequirements.storage.isEmpty() && !m_systemSpecs.contains("Storage")){
            storageStatus = "System Storage Info Not Found";
        } else if (m_gameRequirements.storage.isEmpty() && m_systemSpecs.contains("Storage")){
            storageStatus = "Requirement Not Specified";
        }
        QTreeWidgetItem* storageItem = new QTreeWidgetItem(comparisonTreeWidget, {"Storage", storageStatus, m_systemSpecs.value("StorageDisplay", m_systemSpecs.value("Storage", "N/A")), m_gameRequirements.storage});
        storageItem->setForeground(1, (storageStatus == "Meets or Exceeds") ? QBrush(Qt::green) : (storageStatus == "May Not Meet" ? QBrush(Qt::red) : QBrush(Qt::yellow)));

        
        comparisonTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        comparisonTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        comparisonTreeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        comparisonTreeWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

        qDebug() << "Comparison finished and UI updated";
        qDebug() << "--- END performComparison DEBUG ---";
    }

private:
    QTreeWidget *treeWidget;
    QLabel *statusLabel;
    QThread workerThread;
    DxDiagWorker *worker;
    QLineEdit *gameNameLineEdit;
    QLineEdit *appIdLineEdit;
    
    QThread gameSearchThread;
    GameRequirementsWorker *gameSearchWorker;

    QList<DxDiagSectionData> m_dxdiagData;
    GameRequirements m_gameRequirements;
    QTreeWidget *comparisonTreeWidget;
    QMap<QString, QString> m_systemSpecs; 
    QString m_lastGameName;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    qDebug() << "Application started";

    
    qRegisterMetaType<DxDiagSectionData>("DxDiagSectionData");
    qRegisterMetaType<QList<DxDiagSectionData>>("QList<DxDiagSectionData>");

    QApplication app(argc, argv);
    DxDiagWidget w;
    w.show();
    int ret = app.exec();
    qDebug() << "Application finished with return code" << ret;
    return ret;
} 