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
#if(DEBUG_LOG)
   QString dbgPath = QString() + LOG_ROOT + "dbg_" +  QDateTime::currentDateTime().toString("yyyyMMdd") + ".log";
   writeLog(dbgPath,str);
#else
   (void)str;
#endif
}


void writeWorkLog(QString str)
{
   QString wrkPath = QString() + LOG_ROOT + "work_" +  QDateTime::currentDateTime().toString("yyyyMMdd") + ".log";
   writeLog(wrkPath,str);
}


void writeErrorLog(QString str)
{
   QString errPath = QString() + LOG_ROOT + "error_" +  QDateTime::currentDateTime().toString("yyyyMMdd") + ".log";
   writeLog(errPath,str);
}
