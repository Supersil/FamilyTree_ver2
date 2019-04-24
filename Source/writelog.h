#ifndef WRITELOG_H
#define WRITELOG_H

#include <QString>
#include <QDateTime>

#define LOG_ROOT "Logs/"

void writeLog(QString fileName, QString str);


void writeErrorLog(QString str);
void writeWorkLog(QString str);
void writeDebugLog(QString str);

#endif // WRITELOG_H
