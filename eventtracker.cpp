#include "eventtracker.h"
#include "database.h"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

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

EventTracker::EventTracker(Database *db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
    connect(this, &EventTracker::focusChanged, this, &EventTracker::handleFocusChanged);

    // 60-second crash guard timer
    connect(&m_crashGuardTimer, &QTimer::timeout, this, &EventTracker::flushCurrentInterval);
    m_crashGuardTimer.start(60000);

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
    closeCurrentInterval();

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

QString EventTracker::resolveAppId(quint32 pid)
{
#ifdef Q_OS_WIN
    if (pid == 0) {
        return QString();
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) {
        return QString();
    }

    wchar_t exePath[MAX_PATH] = {0};
    DWORD size = MAX_PATH;
    QString name;
    if (QueryFullProcessImageNameW(hProcess, 0, exePath, &size)) {
        name = QFileInfo(QString::fromWCharArray(exePath, size)).fileName();
    }
    CloseHandle(hProcess);
    return name;
#else
    Q_UNUSED(pid);
    return QString();
#endif
}

void EventTracker::closeCurrentInterval()
{
    if (m_currentIntervalId > 0 && m_db) {
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        m_db->updateIntervalEnd(m_currentIntervalId, now);
    }
    m_currentIntervalId = -1;
    m_currentApp.clear();
    m_currentTitle.clear();
    m_currentStart = 0;
}

void EventTracker::handleFocusChanged(quint32 pid, const QString &title)
{
    // Close the preceding interval
    closeCurrentInterval();

    const QString appId = resolveAppId(pid);

    // Filter out empty titles, explorer.exe, or unknown processes
    if (title.trimmed().isEmpty() || appId.isEmpty() || appId.compare("explorer.exe", Qt::CaseInsensitive) == 0) {
        return;
    }

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    m_currentApp = appId;
    m_currentTitle = title;
    m_currentStart = now;

    if (m_db) {
        // Start interval with fallback end time (+60s) in case of sudden crash
        m_currentIntervalId = m_db->startInterval(m_currentApp, m_currentTitle, m_currentStart, m_currentStart + 60);
    }
}

void EventTracker::flushCurrentInterval()
{
    if (m_currentIntervalId > 0 && m_db) {
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        m_db->updateIntervalEnd(m_currentIntervalId, now);
    }
}
