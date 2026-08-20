#include "norentray.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QApplication>

NorenTray::NorenTray(QObject *parent)
    : QObject(parent)
{
    auto *trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon::fromTheme("dialog-information"));
    trayIcon->setToolTip("noren — tracking your time");

    auto *menu = new QMenu();
    menu->addAction("Quit", qApp, &QApplication::quit);
    trayIcon->setContextMenu(menu);

    trayIcon->show();
}

NorenTray::~NorenTray() = default;

