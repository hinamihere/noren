#include "database.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <iostream>
#include <iomanip>

static void printReportTable(const QList<AppUsageSummary> &report)
{
    if (report.isEmpty()) {
        std::cout << "No tracked activity recorded for today yet." << std::endl;
        return;
    }

    std::cout << std::left << std::setw(30) << "Application" << std::setw(15) << "Time" << std::endl;
    std::cout << std::string(45, '-') << std::endl;

    qint64 totalAllSeconds = 0;
    for (const auto &item : report) {
        totalAllSeconds += item.totalSeconds;
        qint64 hours = item.totalSeconds / 3600;
        qint64 minutes = (item.totalSeconds % 3600) / 60;

        std::string timeStr = std::to_string(hours) + "h " +
                              (minutes < 10 ? "0" : "") + std::to_string(minutes) + "m";

        std::cout << std::left << std::setw(30) << item.appId.toStdString()
                  << std::setw(15) << timeStr << std::endl;
    }

    std::cout << std::string(45, '-') << std::endl;
    qint64 totalHours = totalAllSeconds / 3600;
    qint64 totalMinutes = (totalAllSeconds % 3600) / 60;
    std::string totalTimeStr = std::to_string(totalHours) + "h " +
                               (totalMinutes < 10 ? "0" : "") + std::to_string(totalMinutes) + "m";
    std::cout << std::left << std::setw(30) << "Total"
              << std::setw(15) << totalTimeStr << std::endl;
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
    QCoreApplication::setApplicationName("noren-cli");
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
        std::cerr << "Unknown command: " << cmd.toStdString() << std::endl;
        std::cerr << "Available commands: report" << std::endl;
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
            std::cerr << "Could not connect to noren daemon and failed to open database." << std::endl;
            return 1;
        }
    }

    printReportTable(report);
    return 0;
}
