#include "noren.h"
#include "dashboard.h"
#include "eventtracker.h"
#include "database.h"
#include "utils.h"

#include <QApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QMenu>
#include <QSystemTrayIcon>

Noren::Noren(QObject *parent)
    : QObject(parent)
    , m_db(std::make_unique<Database>(this))
{
    m_db->open();

    m_dashboard = std::make_unique<Dashboard>(m_db.get());

    m_focusTracker = std::make_unique<EventTracker>(m_db.get(), this);
    qApp->installNativeEventFilter(m_focusTracker.get());
    connect(m_focusTracker.get(), &EventTracker::focusChanged, this, &Noren::onFocusChanged);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(Utils::loadIcon("noren.jpg", "dialog-information"));
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

    // Start named pipe / local IPC server
    QLocalServer::removeServer("noren");
    if (!m_server.listen("noren")) {
        qWarning() << "Failed to start local IPC server:" << m_server.errorString();
    } else {
        connect(&m_server, &QLocalServer::newConnection, this, &Noren::onNewConnection);
    }
}

Noren::~Noren()
{
    m_server.close();
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

void Noren::onNewConnection()
{
    QLocalSocket *socket = m_server.nextPendingConnection();
    if (!socket) {
        return;
    }

    connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
        const QByteArray data = socket->readAll();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        QJsonObject response;
        if (!parseError.error && doc.isObject()) {
            const QString cmd = doc.object().value("cmd").toString();
            if (cmd == "report") {
                const auto report = m_db->getReportForToday();
                QJsonArray items;
                for (const auto &item : report) {
                    QJsonObject obj;
                    obj["app_id"] = item.appId;
                    obj["total_seconds"] = item.totalSeconds;
                    items.append(obj);
                }
                response["status"] = "ok";
                response["data"] = items;
            } else {
                response["status"] = "error";
                response["message"] = "Unknown command";
            }
        } else {
            response["status"] = "error";
            response["message"] = "Invalid JSON command";
        }

        connect(socket, &QLocalSocket::bytesWritten, socket, &QLocalSocket::disconnectFromServer);
        socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        socket->flush();
    });

    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}



