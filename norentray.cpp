#include "norentray.h"
#include "dashboard.h"
#include <QMenu>
#include <QApplication>

NorenTray::NorenTray(QObject *parent)
    : QObject(parent)
{
    m_dashboard = new Dashboard();
    m_dashboard->hide();

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon::fromTheme("dialog-information"));
    m_trayIcon->setToolTip("noren");

    auto *menu = new QMenu();
    menu->addAction("Dashboard", this, &NorenTray::showDashboard);
    menu->addSeparator();
    menu->addAction("Quit", qApp, &QApplication::quit);
    m_trayIcon->setContextMenu(menu);
    m_trayIcon->show();
}

NorenTray::~NorenTray() = default;

void NorenTray::showDashboard()
{
    m_dashboard->show();
    m_dashboard->raise();
    m_dashboard->activateWindow();
}

