#include "eventtracker.h"

#ifdef Q_OS_WIN
static EventTracker *s_instance = nullptr;

void CALLBACK EventTracker::winEventProc(
    HWINEVENTHOOK /*hWinEventHook*/,
    DWORD event,
    HWND hwnd,
    LONG /*idObject*/,
    LONG /*idChild*/,
    DWORD /*dwEventThread*/,
    DWORD /*dwmsEventTime*/
)
{
    if (event == EVENT_SYSTEM_FOREGROUND && s_instance && hwnd) {
        s_instance->processWindow(hwnd);
    }
}

void EventTracker::processWindow(HWND hwnd)
{
    if (!hwnd) {
        return;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    wchar_t titleBuffer[512] = {0};
    int len = GetWindowTextW(hwnd, titleBuffer, 512);
    QString title = QString::fromWCharArray(titleBuffer, len);

    emit focusChanged(static_cast<quint32>(pid), title);
}
#endif

EventTracker::EventTracker(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    s_instance = this;
    m_hook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,
        EVENT_SYSTEM_FOREGROUND,
        nullptr,
        &EventTracker::winEventProc,
        0,
        0,
        WINEVENT_OUTOFCONTEXT
    );

    // Initial check for currently active foreground window
    HWND foregroundHwnd = GetForegroundWindow();
    if (foregroundHwnd) {
        processWindow(foregroundHwnd);
    }
#endif
}

EventTracker::~EventTracker()
{
#ifdef Q_OS_WIN
    if (m_hook) {
        UnhookWinEvent(m_hook);
        m_hook = nullptr;
    }
    if (s_instance == this) {
        s_instance = nullptr;
    }
#endif
}

bool EventTracker::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}
