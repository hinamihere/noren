#include "norentray.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NorenTray w;
    return QApplication::exec();
}
