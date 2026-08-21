#include "norentray.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NorenTray tray;
    return QApplication::exec();
}
