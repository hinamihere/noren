#ifndef FOCUSTRACKER_H
#define FOCUSTRACKER_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class FocusTracker : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit FocusTracker(QObject *parent = nullptr);
    ~FocusTracker() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    void focusChanged(quint32 pid, const QString &title);

private:
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

#endif // FOCUSTRACKER_H
