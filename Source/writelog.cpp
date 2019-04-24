#include "writelog.h"
#include <QFile>
#include <QDir>

void writeLog(QString fileName, QString str)
{
   QDir dir;
   dir.mkpath(LOG_ROOT);
   QFile ofile;
   ofile.setFileName(fileName);
   if(!ofile.open(QIODevice::WriteOnly | QIODevice::Append))
      return;
   ofile.write(str.toLocal8Bit());
   ofile.write("\r\n");
   ofile.flush();
   ofile.close();
}



void writeDebugLog(QString str)
{
   QString dbgPath = QString() + LOG_ROOT + "dbg_" +  QDateTime::currentDateTime().toString("yyyyMMdd") + ".log";
   writeLog(dbgPath,str);
}


void writeWorkLog(QString str)
{
   QString dbgPath = QString() + LOG_ROOT + "work_" +  QDateTime::currentDateTime().toString("yyyyMMdd") + ".log";
   writeLog(dbgPath,str);
}


void writeErrorLog(QString str)
{
   QString dbgPath = QString() + LOG_ROOT + "error_" +  QDateTime::currentDateTime().toString("yyyyMMdd") + ".log";
   writeLog(dbgPath,str);
}
