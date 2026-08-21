#include "noren.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Noren tray;
    return QApplication::exec();
}
