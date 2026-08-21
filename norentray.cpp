#include "norentray.h"
#include "dashboard.h"
#include "focustracker.h"

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QSystemTrayIcon>

NorenTray::NorenTray(QObject *parent)
    : QObject(parent)
    , m_dashboard(std::make_unique<Dashboard>())
    , m_focusTracker(std::make_unique<FocusTracker>(this))
{
    qApp->installNativeEventFilter(m_focusTracker.get());
    connect(m_focusTracker.get(), &FocusTracker::focusChanged, this, &NorenTray::onFocusChanged);

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

NorenTray::~NorenTray()
{
    if (m_focusTracker) {
        qApp->removeNativeEventFilter(m_focusTracker.get());
    }
}

void NorenTray::showDashboard()
{
    m_dashboard->show();
    m_dashboard->raise();
    m_dashboard->activateWindow();
}

void NorenTray::onFocusChanged(quint32 pid, const QString &title)
{
    qDebug() << "Focus:" << pid << title;
}


