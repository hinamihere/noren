#ifndef EVENTTRACKER_H
#define EVENTTRACKER_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QString>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class Database;

class EventTracker : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit EventTracker(Database *db, QObject *parent = nullptr);
    ~EventTracker() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    void focusChanged(quint32 pid, const QString &title);

private slots:
    void handleFocusChanged(quint32 pid, const QString &title);
    void flushCurrentInterval();

private:
    Database *m_db{nullptr};
    QTimer m_crashGuardTimer;

    QString m_currentApp;
    QString m_currentTitle;
    qint64 m_currentStart{0};
    qint64 m_currentIntervalId{-1};

    void closeCurrentInterval();
    QString resolveAppId(quint32 pid);

#ifdef Q_OS_WIN
    HWINEVENTHOOK m_hook{nullptr};
    static void CALLBACK winEventProc(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD dwEventThread,
        DWORD dwmsEventTime
    );
    void processWindow(HWND hwnd);
#endif
};

#endif // EVENTTRACKER_H
