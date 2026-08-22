#include "config.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>

Config::Config(QObject *parent)
    : QObject(parent)
{
}

QString Config::defaultConfigPath()
{
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appDataDir + "/config.toml";
}

QString Config::configPath() const
{
    return m_configPath.isEmpty() ? defaultConfigPath() : m_configPath;
}

qint64 Config::parseTimeString(const QString &timeStr)
{
    // Parse "2h", "90m", "1h30m", "30s"
    QRegularExpression re(R"((?:(\d+)h)?(?:(\d+)m)?(?:(\d+)s)?)");
    QRegularExpressionMatch match = re.match(timeStr.trimmed());

    if (!match.hasMatch()) {
        return 0;
    }

    qint64 seconds = 0;
    if (!match.captured(1).isEmpty()) {
        seconds += match.captured(1).toLongLong() * 3600;
    }
    if (!match.captured(2).isEmpty()) {
        seconds += match.captured(2).toLongLong() * 60;
    }
    if (!match.captured(3).isEmpty()) {
        seconds += match.captured(3).toLongLong();
    }

    return seconds;
}

void Config::parseTOML(const QString &content)
{
    m_limits.clear();

    QTextStream stream(const_cast<QString *>(&content));
    bool inLimitsSection = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        // Section header
        if (line.startsWith('[') && line.endsWith(']')) {
            QString section = line.mid(1, line.length() - 2).trimmed();
            inLimitsSection = (section == "limits");
            continue;
        }

        // Key = value pairs
        if (inLimitsSection && line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString key = line.left(eqPos).trimmed();
            QString value = line.mid(eqPos + 1).trimmed();

            // Remove quotes if present
            if ((value.startsWith('"') && value.endsWith('"')) ||
                (value.startsWith('\'') && value.endsWith('\''))) {
                value = value.mid(1, value.length() - 2);
            }

            // Parse time string
            qint64 seconds = parseTimeString(value);
            if (seconds > 0) {
                m_limits[key.toLower()] = seconds;
                qDebug() << "Loaded limit:" << key << "=" << seconds << "seconds";
            }
        }
    }
}

QString Config::generateTOML() const
{
    QString toml;
    toml += "# noren configuration file\n";
    toml += "# https://github.com/hinamihere/noren\n";
    toml += "\n";
    toml += "# App usage limits\n";
    toml += "# Format: app_name = \"Xh\" or \"Xm\" or \"Xh Ym\"\n";
    toml += "# Examples:\n";
    toml += "#   firefox = \"2h\"\n";
    toml += "#   discord = \"90m\"\n";
    toml += "#   youtube = \"1h 30m\"\n";
    toml += "\n";
    toml += "[limits]\n";

    if (m_limits.isEmpty()) {
        toml += "# firefox = \"2h\"\n";
        toml += "# discord = \"90m\"\n";
        toml += "# chrome = \"1h 30m\"\n";
    } else {
        QMapIterator<QString, qint64> it(m_limits);
        while (it.hasNext()) {
            it.next();
            qint64 secs = it.value();
            qint64 hours = secs / 3600;
            qint64 mins = (secs % 3600) / 60;

            QString timeStr;
            if (hours > 0 && mins > 0) {
                timeStr = QString("%1h %2m").arg(hours).arg(mins);
            } else if (hours > 0) {
                timeStr = QString("%1h").arg(hours);
            } else {
                timeStr = QString("%1m").arg(mins);
            }

            toml += QString("%1 = \"%2\"\n").arg(it.key()).arg(timeStr);
        }
    }

    return toml;
}

bool Config::load(const QString &path)
{
    m_configPath = path.isEmpty() ? defaultConfigPath() : path;

    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config file:" << m_configPath;
        return false;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    parseTOML(content);
    qDebug() << "Loaded" << m_limits.size() << "limits from" << m_configPath;
    return true;
}

bool Config::save(const QString &path)
{
    QString savePath = path.isEmpty() ? configPath() : path;

    // Ensure directory exists
    QFileInfo fileInfo(savePath);
    QDir().mkpath(fileInfo.absolutePath());

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to write config file:" << savePath;
        return false;
    }

    QTextStream stream(&file);
    stream << generateTOML();
    file.close();

    m_configPath = savePath;
    qDebug() << "Saved config to" << savePath;
    return true;
}

bool Config::initConfig(const QString &path)
{
    QString initPath = path.isEmpty() ? defaultConfigPath() : path;

    // Check if config already exists
    if (QFile::exists(initPath)) {
        qWarning() << "Config file already exists at" << initPath;
        return false;
    }

    m_configPath = initPath;
    return save(initPath);
}

qint64 Config::limitForApp(const QString &appId) const
{
    return m_limits.value(appId.toLower(), 0);
}

bool Config::hasLimit(const QString &appId) const
{
    return m_limits.contains(appId.toLower());
}
