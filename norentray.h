#ifndef NORENTRAY_H
#define NORENTRAY_H

#include <QObject>
#include <memory>

class QSystemTrayIcon;
class Dashboard;

class NorenTray : public QObject
{
    Q_OBJECT

public:
    explicit NorenTray(QObject *parent = nullptr);
    ~NorenTray() override;

private slots:
    void showDashboard();

private:
    std::unique_ptr<Dashboard> m_dashboard;
    QSystemTrayIcon *m_trayIcon{nullptr};
};

#endif // NORENTRAY_H
