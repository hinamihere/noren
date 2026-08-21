#ifndef NOREN_H
#define NOREN_H

#include <QObject>
#include <QLocalServer>
#include <memory>

class QSystemTrayIcon;
class Dashboard;
class EventTracker;
class Database;

class Noren : public QObject
{
    Q_OBJECT

public:
    explicit Noren(QObject *parent = nullptr);
    ~Noren() override;

private slots:
    void showDashboard();
    void onFocusChanged(quint32 pid, const QString &title);
    void onNewConnection();

private:
    std::unique_ptr<Database> m_db;
    std::unique_ptr<Dashboard> m_dashboard;
    std::unique_ptr<EventTracker> m_focusTracker;
    QSystemTrayIcon *m_trayIcon{nullptr};
    QLocalServer m_server;
};

#endif // NOREN_H
