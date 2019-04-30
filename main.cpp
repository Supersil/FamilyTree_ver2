#include "familytreewidget.h"
#include <QApplication>

#include "writelog.h"
#include "db.h"
#include "person.h"

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

   Person me;
   me.name = "Силков Александр Андреевич";
   me.bIsAlive = true;
   me.birthDate = QDate(1992,06,12);
   me.info = "Создатель данной нетленки";
   me.birthPlace = "Москва";
   me.sex = "Мужской";


   if (db.addPerson("SILKOVAA",1,me.name.toStdString(),me.birthDate.toString("dd.MM.yyyy").toStdString(),me.bIsAlive?"Alive":"Dead","",me.info.toStdString(),me.birthPlace.toStdString(),"",me.sex.toStdString(),(me.father != nullptr)?me.father->id:-1,(me.mother != nullptr)?me.mother->id:-1,0,""))
   {
      qDebug() << "Failed to add person to db";
      return -1;
   }

   me.name = "Абрахманова Марианна Александровна";
   me.bIsAlive = true;
   me.birthDate = QDate(1992,04,21);
   me.info = "ЛюбоФ";
   me.birthPlace = "Москва";
   me.sex = "Женский";


   if (db.addPerson("SILKOVAA",2,me.name.toStdString(),me.birthDate.toString("dd.MM.yyyy").toStdString(),me.bIsAlive?"Alive":"Dead","",me.info.toStdString(),me.birthPlace.toStdString(),"",me.sex.toStdString(),(me.father != nullptr)?me.father->id:-1,(me.mother != nullptr)?me.mother->id:-1,0,""))
   {
      qDebug() << "Failed to add person to db";
      return -1;
   }

   std::vector<std::string> roots, tables;

   if(db.getListOfRoots(roots,tables))
   {
      qDebug() << "Failed to get list of roots";
      return -1;
   }

   if (db.closeDB())
   {
      qDebug() << "Failed to close db";
      return -1;
   }

   return 1;
}
