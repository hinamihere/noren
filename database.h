#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QSqlDatabase>

struct IntervalRecord {
    qint64 id{0};
    QString appId;
    QString title;
    qint64 started{0};
    qint64 ended{0};
    int idle{0};
};

struct AppUsageSummary {
    QString appId;
    qint64 totalSeconds{0};
};

class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database() override;

    bool open(const QString &path = QString());
    void close();

    qint64 startInterval(const QString &appId, const QString &title, qint64 started, qint64 ended = 0, int idle = 0);
    bool updateIntervalEnd(qint64 id, qint64 ended);

    QList<AppUsageSummary> getReportForToday();
    void requestReportForToday();
    void recoverOrphanedIntervals();

signals:
    void reportReady(const QList<AppUsageSummary> &report);

private:
    QSqlDatabase m_db;
};

#endif // DATABASE_H
