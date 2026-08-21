#include "noren.h"
#include "dashboard.h"
#include "eventtracker.h"

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QSystemTrayIcon>

Noren::Noren(QObject *parent)
    : QObject(parent)
    , m_dashboard(std::make_unique<Dashboard>())
    , m_focusTracker(std::make_unique<EventTracker>(this))
{
    qApp->installNativeEventFilter(m_focusTracker.get());
    connect(m_focusTracker.get(), &EventTracker::focusChanged, this, &Noren::onFocusChanged);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon::fromTheme("dialog-information"));
    m_trayIcon->setToolTip("noren");

    auto *menu = new QMenu();
    menu->addAction("Dashboard", this, &Noren::showDashboard);
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

Noren::~Noren()
{
    if (m_focusTracker) {
        qApp->removeNativeEventFilter(m_focusTracker.get());
    }
}

void Noren::showDashboard()
{
    m_dashboard->show();
    m_dashboard->raise();
    m_dashboard->activateWindow();
}

void Noren::onFocusChanged(quint32 pid, const QString &title)
{
    qDebug() << "Focus:" << pid << title;
}


