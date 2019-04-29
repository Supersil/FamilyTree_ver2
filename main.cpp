#include "familytreewidget.h"
#include <QApplication>

#include "writelog.h"
#include "db.h"

#include <QDebug>
#include <QTextCodec>

int main(int argc, char *argv[])
{
   QApplication a(argc, argv);
   QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

//   FamilyTreeWidget w;
//   w.show();

   DB db("tmp.db");

   if (db.openDB())
   {
      qDebug() << "Failed to open db";
      return -1;
   }

   if (db.checkDB())
   {
      qDebug() << "Failed to check db";
      return -1;
   }

   if (db.createTables())
   {
      qDebug() << "Failed to create tables";
      return -1;
   }

   if (db.createRoot("Силков Александр Андреевич","SILKOVAA"))
   {
      qDebug() << "Failed to create root";
      return -1;
   }

   Person pers(QDate(1992,06,12),"Силков Александр Андреевич","Создатель данной нетленки","Москва","",sexx::MALE);

   if (db.addPerson("SILKOVAA",&pers))
   {
      qDebug() << "Failed to add person to db";
      return -1;
   }

   if (db.closeDB())
   {
      qDebug() << "Failed to close db";
      return -1;
   }

   return 1;
}
