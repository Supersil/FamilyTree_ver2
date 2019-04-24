#include "familytreewidget.h"
#include <QApplication>

#include "writelog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    FamilyTreeWidget w;
    w.show();

    return a.exec();
}
