#include "database.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QThread>
#include <QThreadPool>

Database::Database(QObject *parent)
    : QObject(parent)
{
}

Database::~Database()
{
    close();
}

bool Database::open(const QString &path)
{
    QString dbPath = path;
    if (dbPath.isEmpty()) {
        const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appDataDir);
        dbPath = appDataDir + "/noren.db";
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open SQLite database at" << dbPath << ":" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query;
    const QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS intervals (
            id INTEGER PRIMARY KEY,
            app_id TEXT NOT NULL,
            title TEXT,
            started INTEGER NOT NULL,
            ended INTEGER NOT NULL,
            idle INTEGER NOT NULL DEFAULT 0
        );
    )";

    if (!query.exec(createTableSql)) {
        qWarning() << "Failed to create intervals table:" << query.lastError().text();
        return false;
    }

    const QString createIndexSql = R"(
        CREATE INDEX IF NOT EXISTS idx_intervals_started ON intervals(started);
    )";

    if (!query.exec(createIndexSql)) {
        qWarning() << "Failed to create index on intervals(started):" << query.lastError().text();
        return false;
    }

    recoverOrphanedIntervals();
    return true;
}

void Database::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

qint64 Database::startInterval(const QString &appId, const QString &title, qint64 started, qint64 ended, int idle)
{
    if (!m_db.isOpen()) {
        return -1;
    }

    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO intervals (app_id, title, started, ended, idle)
        VALUES (:app_id, :title, :started, :ended, :idle)
    )");
    query.bindValue(":app_id", appId);
    query.bindValue(":title", title);
    query.bindValue(":started", started);
    query.bindValue(":ended", ended);
    query.bindValue(":idle", idle);

    if (!query.exec()) {
        qWarning() << "Failed to insert interval:" << query.lastError().text();
        return -1;
    }

    return query.lastInsertId().toLongLong();
}

bool Database::updateIntervalEnd(qint64 id, qint64 ended)
{
    if (!m_db.isOpen() || id <= 0) {
        return false;
    }

    QSqlQuery query;
    query.prepare("UPDATE intervals SET ended = :ended WHERE id = :id");
    query.bindValue(":ended", ended);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "Failed to update interval end:" << query.lastError().text();
        return false;
    }

    return true;
}

QList<AppUsageSummary> Database::getReportForToday()
{
    QList<AppUsageSummary> summaries;
    if (!m_db.isOpen()) {
        return summaries;
    }

    const qint64 todayStart = QDateTime::currentDateTime().date().startOfDay().toSecsSinceEpoch();

    QSqlQuery query;
    query.prepare(R"(
        SELECT app_id, SUM(ended - started) as total_seconds
        FROM intervals
        WHERE started >= :todayStart
        GROUP BY app_id
        ORDER BY total_seconds DESC
    )");
    query.bindValue(":todayStart", todayStart);

    if (!query.exec()) {
        qWarning() << "Failed to query today's report:" << query.lastError().text();
        return summaries;
    }

    while (query.next()) {
        AppUsageSummary summary;
        summary.appId = query.value("app_id").toString();
        summary.totalSeconds = query.value("total_seconds").toLongLong();
        summaries.append(summary);
    }

    return summaries;
}

void Database::requestReportForToday()
{
    const QString dbPath = m_db.databaseName();
    QThreadPool::globalInstance()->start([this, dbPath]() {
        const QString connName = QString("async_report_%1").arg(quintptr(QThread::currentThreadId()));
        QList<AppUsageSummary> summaries;
        {
            QSqlDatabase threadDb = QSqlDatabase::addDatabase("QSQLITE", connName);
            threadDb.setDatabaseName(dbPath);
            if (threadDb.open()) {
                const qint64 todayStart = QDateTime::currentDateTime().date().startOfDay().toSecsSinceEpoch();
                QSqlQuery query(threadDb);
                query.prepare(R"(
                    SELECT app_id, SUM(ended - started) as total_seconds
                    FROM intervals
                    WHERE started >= :todayStart
                    GROUP BY app_id
                    ORDER BY total_seconds DESC
                )");
                query.bindValue(":todayStart", todayStart);

                if (query.exec()) {
                    while (query.next()) {
                        AppUsageSummary summary;
                        summary.appId = query.value("app_id").toString();
                        summary.totalSeconds = query.value("total_seconds").toLongLong();
                        summaries.append(summary);
                    }
                } else {
                    qWarning() << "Async report query failed:" << query.lastError().text();
                }
                threadDb.close();
            }
        }
        QSqlDatabase::removeDatabase(connName);

        QMetaObject::invokeMethod(this, [this, summaries]() {
            emit reportReady(summaries);
        }, Qt::QueuedConnection);
    });
}

void Database::recoverOrphanedIntervals()
{
    if (!m_db.isOpen()) {
        return;
    }

    const qint64 now = QDateTime::currentSecsSinceEpoch();

    // Close any unclosed or future-ended intervals at started + 60s
    QSqlQuery query;
    query.prepare(R"(
        UPDATE intervals
        SET ended = started + 60
        WHERE ended = 0 OR ended < started OR ended > :now;
    )");
    query.bindValue(":now", now);

    if (!query.exec()) {
        qWarning() << "Failed to run startup recovery for orphaned intervals:" << query.lastError().text();
    } else if (query.numRowsAffected() > 0) {
        qDebug() << "Recovered" << query.numRowsAffected() << "orphaned interval(s).";
    }
}
