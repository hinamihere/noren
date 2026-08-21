#include "noren.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName("noren");
    QApplication::setApplicationName("noren");
    Noren tray;
    return QApplication::exec();
}
