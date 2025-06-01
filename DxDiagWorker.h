#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <thread>
#include <chrono>
#include <fstream>
#include <QProcess>
#include <QXmlStreamReader>


struct DxDiagSectionData {
    QString sectionName;
    QList<QStringList> items;
};

Q_DECLARE_METATYPE(DxDiagSectionData)
Q_DECLARE_METATYPE(QList<DxDiagSectionData>)

class DxDiagWorker : public QObject
{
    Q_OBJECT

public:
    DxDiagWorker(QObject *parent = nullptr) : QObject(parent) { qDebug() << "DxDiagWorker created"; }

    ~DxDiagWorker() override { qDebug() << "DxDiagWorker destroyed"; }

public slots:
    void processDxDiag()
    {
        qDebug() << "DxDiagWorker::processDxDiag started";
        emit started();
        qDebug() << "DxDiagWorker::started() emitted";

        
        QString outputFile = "dxdiag_output.xml";
        QString program = "dxdiag.exe";
        QStringList arguments;
        arguments << "/x" << outputFile;

        qDebug() << "Running command:" << program << arguments;
        QProcess dxdiagProcess;
        dxdiagProcess.start(program, arguments);
        dxdiagProcess.waitForFinished(-1); 

        if (dxdiagProcess.exitCode() != 0) {
            qDebug() << "Error running dxdiag.exe:";
            qDebug() << dxdiagProcess.readAllStandardError();
            emit error("Failed to run dxdiag.exe");
            emit finished();
            return;
        }
        qDebug() << "dxdiag.exe finished successfully.";

        
        QFile file(outputFile);
        QList<DxDiagSectionData> sectionsData;

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error: Could not open" << outputFile;
            emit error("Could not open " + outputFile);
            emit finished();
            qDebug() << "DxDiagWorker::error() and finished() emitted due to file error";
            return;
        }

        qDebug() << "File" << outputFile << "opened successfully";
        QXmlStreamReader xml(&file);

        
        if (xml.readNextStartElement() && xml.name() == "DxDiag") {
            qDebug() << "Found DxDiag root element.";

            
            while (!xml.atEnd() && !xml.hasError()) {
                QXmlStreamReader::TokenType token = xml.readNext();

                if (token == QXmlStreamReader::StartElement) {
                    QString elementName = xml.name().toString();

                    if (elementName == "SystemInformation") {
                        qDebug() << "Started parsing SystemInformation section.";
                        DxDiagSectionData systemInfoSection;
                        systemInfoSection.sectionName = "SystemInformation";

                        while (!xml.atEnd() && !xml.hasError() && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "SystemInformation")) {
                            if (xml.readNext() == QXmlStreamReader::StartElement) {
                                QString key = xml.name().toString();
                                if (key == "Processor" || key == "Memory") {
                                    QString value = xml.readElementText();
                                    if (!value.isEmpty()) {
                                        systemInfoSection.items.append({key, value});
                                        qDebug() << "Extracted SystemInformation item:" << key << ":" << value;
                                    }
                                } else {
                                    xml.skipCurrentElement(); 
                                }
                            }
                        }
                         
                        if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "SystemInformation") {
                             qDebug() << "Finished parsing SystemInformation section.";
                             sectionsData.append(systemInfoSection);
                             qDebug() << "Appended SystemInformation section with" << systemInfoSection.items.size() << "items. Total sections now:" << sectionsData.size();
                        }

                    } else if (elementName == "DisplayDevices") {
                        qDebug() << "Started parsing DisplayDevices section.";
                        DxDiagSectionData displayDevicesSection;
                        displayDevicesSection.sectionName = "DisplayDevices";

                        
                        while (!xml.atEnd() && !xml.hasError() && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "DisplayDevices")) {
                            if (xml.readNext() == QXmlStreamReader::StartElement) {
                                if (xml.name() == "DisplayDevice") {
                                    qDebug() << "Found nested DisplayDevice.";
                                    
                                    while (!xml.atEnd() && !xml.hasError() && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "DisplayDevice")) {
                                        if (xml.readNext() == QXmlStreamReader::StartElement) {
                                            QString itemKey = xml.name().toString();
                                            if (itemKey == "CardName" || itemKey == "CurrentMode") {
                                                QString itemValue = xml.readElementText();
                                                if (!itemValue.isEmpty()) {
                                                     displayDevicesSection.items.append({itemKey, itemValue});
                                                     qDebug() << "Extracted DisplayDevices item:" << itemKey << ":" << itemValue;
                                                }
                                            } else {
                                                xml.skipCurrentElement(); 
                                            }
                                        }
                                    }
                                    
                                    if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "DisplayDevice") {
                                        qDebug() << "Finished parsing nested DisplayDevice.";
                                    }
                                } else {
                                    xml.skipCurrentElement(); 
                                }
                            }
                        }
                         
                        if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "DisplayDevices") {
                             qDebug() << "Finished parsing DisplayDevices section.";
                             sectionsData.append(displayDevicesSection);
                             qDebug() << "Appended DisplayDevices section with" << displayDevicesSection.items.size() << "items. Total sections now:" << sectionsData.size();
                        }

                    } else if (elementName == "LogicalDisks") {
                        qDebug() << "Started parsing LogicalDisks section.";
                        DxDiagSectionData logicalDisksSection;
                        logicalDisksSection.sectionName = "LogicalDisks";

                        
                        while (!xml.atEnd() && !xml.hasError() && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "LogicalDisks")) {
                            if (xml.readNext() == QXmlStreamReader::StartElement) {
                                if (xml.name() == "LogicalDisk") {
                                    qDebug() << "Found nested LogicalDisk.";
                                    QString drive = "N/A";
                                    QString freeSpace = "N/A";
                                    QString size = "N/A";

                                    
                                    while (!xml.atEnd() && !xml.hasError() && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "LogicalDisk")) {
                                         if (xml.readNext() == QXmlStreamReader::StartElement) {
                                            QString itemKey = xml.name().toString();
                                            QString itemValue = xml.readElementText();

                                            if (itemKey == "DriveLetter") {
                                                drive = itemValue;
                                                qDebug() << "Extracted DriveLetter:" << drive;
                                            } else if (itemKey == "FreeSpace") {
                                                freeSpace = itemValue;
                                                qDebug() << "Extracted FreeSpace:" << freeSpace;
                                            } else if (itemKey == "MaxSpace") {
                                                size = itemValue;
                                                qDebug() << "Extracted MaxSpace:" << size;
                                            }
                                        }
                                    }
                                    
                                    if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "LogicalDisk") {
                                        qDebug() << "Finished parsing nested LogicalDisk.";
                                        
                                        logicalDisksSection.items.append({"Drive: " + drive, "Free Space: " + freeSpace, "Size: " + size});
                                        qDebug() << "Added LogicalDisk item with" << logicalDisksSection.items.last().size() << "parts.";
                                    }
                                }
                            }
                        }
                        
                        if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "LogicalDisks") {
                             qDebug() << "Finished parsing LogicalDisks section.";
                             sectionsData.append(logicalDisksSection);
                             qDebug() << "Appended LogicalDisks section with" << logicalDisksSection.items.size() << "items.";
                        }

                    } else {
                        xml.skipCurrentElement(); 
                         qDebug() << "Skipping section:" << elementName;
                    }

                } else if (token == QXmlStreamReader::EndElement && xml.name() == "DxDiag") {
                     qDebug() << "Finished parsing DxDiag root element and main loop.";
                     break; 
                } else if (token == QXmlStreamReader::Characters && !xml.isWhitespace()) {
                    
                    qDebug() << "Ignoring unexpected character data:" << xml.text();
                } else if (token != QXmlStreamReader::Characters && !xml.isWhitespace()) {
                    
                    qDebug() << "Skipping unexpected token between sections: type" << token << ", name:" << xml.name();
                    xml.skipCurrentElement(); 
                }
                
            }
        } else {
            qDebug() << "Error: Could not find DxDiag root element.";
            emit error("Could not find DxDiag root element in XML.");
            emit finished();
            return;
        }

        if (xml.hasError()) {
            qDebug() << "XML parsing error:" << xml.errorString();
            emit error("XML parsing error: " + xml.errorString());
            emit finished();
            return;
        }

        file.close();
        qDebug() << "Finished parsing" << outputFile;

        
        for (const auto& section : sectionsData) {
            if (section.sectionName == "SystemInformation") { 
                qDebug() << "Pre-emit - System Information items count:" << section.items.size();
            } else if (section.sectionName == "DisplayDevices") { 
                qDebug() << "Pre-emit - Display Devices items count:" << section.items.size();
            } else if (section.sectionName == "LogicalDisks") { 
                qDebug() << "Pre-emit - Logical Disks items count:" << section.items.size();
            }
        }

        qDebug() << "Before emitting parsingFinished, sectionsData size:" << sectionsData.size();
        emit parsingFinished(sectionsData);
        qDebug() << "DxDiagWorker::parsingFinished() emitted with" << sectionsData.size() << "sections";
        emit finished();
        qDebug() << "DxDiagWorker::finished() emitted";
    }

signals:
    void started();
    void finished();
    void error(const QString &message);
    void parsingFinished(const QList<DxDiagSectionData> &sectionsData);

private:
    
}; 