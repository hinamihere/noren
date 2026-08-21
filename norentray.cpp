#include "norentray.h"
#include "dashboard.h"

#include <QApplication>
#include <QMenu>
#include <QSystemTrayIcon>

NorenTray::NorenTray(QObject *parent)
    : QObject(parent)
    , m_dashboard(std::make_unique<Dashboard>())
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon::fromTheme("dialog-information"));
    m_trayIcon->setToolTip("noren");

    auto *menu = new QMenu();
    menu->addAction("Dashboard", this, &NorenTray::showDashboard);
    menu->addSeparator();
    menu->addAction("Quit", qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(menu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            showDashboard();
        }
    });

    m_trayIcon->show();
}

NorenTray::~NorenTray() = default;

void NorenTray::showDashboard()
{
    m_dashboard->show();
    m_dashboard->raise();
    m_dashboard->activateWindow();
}

