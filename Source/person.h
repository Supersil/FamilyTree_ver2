#ifndef PERSON_H
#define PERSON_H

#include <QObject>
#include <QDate>
#include <QString>
#include <QVector>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

enum sexx{ MALE, FEMALE};

class Person: public QObject
{
   Q_OBJECT
private:
   int id;
   QString name;
   QDate birthDate;
   bool bIsAlive;
   QDate deathDate;
   QString info;
   QString birthPlace;
   QByteArray photoData;

   sexx sex;
   Person * father;
   Person * mother;
   QVector<Person*> children;
   static int global_id;
   int new_id();

public:
   Person();
   ~Person();
   Person(const Person& src);
   Person(QDate birth, QDate death, bool alive, QString n_name, QString n_info, QString n_birthPlace, QString photopath,sexx s);
   Person(QDate birth, QString n_name, QString n_info, QString n_birthPlace, QString photopath, sexx s);
   void setMother(Person * mom);
   void setFather(Person * dad);
   QString getName();
   void addChild(Person * child);
   int get_id();
   QDate getBDate();
   QDate getDDate();
   QString getBirthPlace();
   QString getInfo();
   int children_num();
   Person * mom();
   Person * dad();
   Person * child(int num);
   QByteArray getPhotoData();
   bool isAlive();
   bool set;
   sexx getSex();
   void save_pure(QString filename);

   QPointF m_coord;

public slots:
   void import_data(Person * profile);
};

#endif // PERSON_H
