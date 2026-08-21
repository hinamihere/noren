#ifndef NORENTRAY_H
#define NORENTRAY_H

#include <QObject>
#include <QSystemTrayIcon>
#include "dashboard.h"

class NorenTray : public QObject
{
    Q_OBJECT

public:
    explicit NorenTray(QObject *parent = nullptr);
    ~NorenTray() override;

private slots:
    void showDashboard();

private:
    QSystemTrayIcon *m_trayIcon;
    Dashboard *m_dashboard;

};
#endif // NORENTRAY_H
