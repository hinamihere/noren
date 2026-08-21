#include "database.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QTextStream>

static void printReportTable(const QList<AppUsageSummary> &report)
{
    QTextStream out(stdout);
    if (report.isEmpty()) {
        out << "No tracked activity recorded for today yet.\n";
        out.flush();
        return;
    }

    out << QString("%1 %2\n").arg("Application", -30).arg("Time", -15);
    out << QString(45, '-') << "\n";

    qint64 totalAllSeconds = 0;
    for (const auto &item : report) {
        totalAllSeconds += item.totalSeconds;
        qint64 hours = item.totalSeconds / 3600;
        qint64 minutes = (item.totalSeconds % 3600) / 60;

        QString timeStr = QString("%1h %2m")
                              .arg(hours)
                              .arg(minutes, 2, 10, QChar('0'));

        out << QString("%1 %2\n").arg(item.appId, -30).arg(timeStr, -15);
    }

    out << QString(45, '-') << "\n";
    qint64 totalHours = totalAllSeconds / 3600;
    qint64 totalMinutes = (totalAllSeconds % 3600) / 60;
    QString totalTimeStr = QString("%1h %2m")
                               .arg(totalHours)
                               .arg(totalMinutes, 2, 10, QChar('0'));
    out << QString("%1 %2\n").arg("Total", -30).arg(totalTimeStr, -15);
    out.flush();
}

static bool fetchReportFromDaemon(QList<AppUsageSummary> &outReport)
{
    QLocalSocket socket;
    socket.connectToServer("noren");
    if (!socket.waitForConnected(1000)) {
        return false;
    }

    QJsonObject request;
    request["cmd"] = "report";
    socket.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    socket.flush();

    if (!socket.waitForReadyRead(3000)) {
        return false;
    }

    QByteArray responseData = socket.readAll();
    while (socket.waitForReadyRead(200)) {
        responseData.append(socket.readAll());
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    if (root.value("status").toString() != "ok") {
        return false;
    }

    QJsonArray items = root.value("data").toArray();
    for (const auto &val : items) {
        QJsonObject obj = val.toObject();
        AppUsageSummary summary;
        summary.appId = obj.value("app_id").toString();
        summary.totalSeconds = obj.value("total_seconds").toVariant().toLongLong();
        outReport.append(summary);
    }

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("noren");
    QCoreApplication::setApplicationName("noren");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("noren CLI — query activity reports and tracking status");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("command", "Command to execute (e.g. 'report')");

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    QString cmd = "report";
    if (!args.isEmpty()) {
        cmd = args.first();
    }

    if (cmd != "report") {
        QTextStream err(stderr);
        err << "Unknown command: " << cmd << "\n";
        err << "Available commands: report\n";
        err.flush();
        return 1;
    }

    QList<AppUsageSummary> report;
    // 1. Try to fetch from active daemon pipe
    if (!fetchReportFromDaemon(report)) {
        // 2. Fallback: Query SQLite database directly if daemon is not running
        Database db;
        if (db.open()) {
            report = db.getReportForToday();
        } else {
            QTextStream err(stderr);
            err << "Could not connect to noren daemon and failed to open database.\n";
            err.flush();
            return 1;
        }
    }

    printReportTable(report);
    return 0;
}
