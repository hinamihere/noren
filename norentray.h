#ifndef NORENTRAY_H
#define NORENTRAY_H

#include <QObject>
#include <memory>

class QSystemTrayIcon;
class Dashboard;
class FocusTracker;

class NorenTray : public QObject
{
    Q_OBJECT

public:
    explicit NorenTray(QObject *parent = nullptr);
    ~NorenTray() override;

private slots:
    void showDashboard();
    void onFocusChanged(quint32 pid, const QString &title);

private:
    std::unique_ptr<Dashboard> m_dashboard;
    std::unique_ptr<FocusTracker> m_focusTracker;
    QSystemTrayIcon *m_trayIcon{nullptr};
};

#endif // NORENTRAY_H
