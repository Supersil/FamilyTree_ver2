#include "familytreewidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FamilyTreeWidget w;
    w.show();

    return a.exec();
}
