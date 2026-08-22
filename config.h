#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QString>
#include <QMap>

struct AppLimit {
    QString appId;
    qint64 limitSeconds{0};  // 0 = no limit
};

class Config : public QObject
{
    Q_OBJECT
public:
    explicit Config(QObject *parent = nullptr);

    bool load(const QString &path = QString());
    bool save(const QString &path = QString());
    bool initConfig(const QString &path = QString());

    QString configPath() const;

    QMap<QString, qint64> limits() const { return m_limits; }
    qint64 limitForApp(const QString &appId) const;
    bool hasLimit(const QString &appId) const;

    static QString defaultConfigPath();
    static qint64 parseTimeString(const QString &timeStr);

private:
    QMap<QString, qint64> m_limits;
    QString m_configPath;

    void parseTOML(const QString &content);
    QString generateTOML() const;
};

#endif // CONFIG_H
