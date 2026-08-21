#include "noren.h"

#include <QApplication>
#include <QColor>
#include <QPalette>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName("noren");
    QApplication::setApplicationName("noren");

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(0x14, 0x14, 0x0D));
    darkPalette.setColor(QPalette::WindowText, QColor(0xE6, 0xE2, 0xD7));
    darkPalette.setColor(QPalette::Base, QColor(0x14, 0x14, 0x0D));
    darkPalette.setColor(QPalette::AlternateBase, QColor(0x21, 0x20, 0x19));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(0xE6, 0xE2, 0xD7));
    darkPalette.setColor(QPalette::ToolTipText, QColor(0x14, 0x14, 0x0D));
    darkPalette.setColor(QPalette::Text, QColor(0xE6, 0xE2, 0xD7));
    darkPalette.setColor(QPalette::Button, QColor(0x21, 0x20, 0x19));
    darkPalette.setColor(QPalette::ButtonText, QColor(0xE6, 0xE2, 0xD7));
    darkPalette.setColor(QPalette::Highlight, QColor(0xE6, 0x8A, 0x5C));
    darkPalette.setColor(QPalette::HighlightedText, QColor(0x14, 0x14, 0x0D));
    QApplication::setPalette(darkPalette);

    Noren tray;
    return QApplication::exec();
}
