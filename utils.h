#ifndef UTILS_H
#define UTILS_H

#include <QCoreApplication>
#include <QFile>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QStringList>

namespace Utils {

inline QString findAssetPath(const QString &fileName)
{
    const QStringList candidates = {
        QString(":/assets/%1").arg(fileName),
        QString(":/%1").arg(fileName),
        QString("assets/%1").arg(fileName),
        fileName,
        QCoreApplication::applicationDirPath() + "/assets/" + fileName,
        QCoreApplication::applicationDirPath() + "/" + fileName
    };

    for (const QString &path : candidates) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    return QString();
}

inline QIcon loadIcon(const QString &fileName, const QString &themeFallback = QString())
{
    const QString path = findAssetPath(fileName);
    if (!path.isEmpty()) {
        return QIcon(path);
    }
    if (!themeFallback.isEmpty()) {
        return QIcon::fromTheme(themeFallback);
    }
    return QIcon();
}

inline QPixmap loadPixmap(const QString &fileName)
{
    const QString path = findAssetPath(fileName);
    if (!path.isEmpty()) {
        return QPixmap(path);
    }
    return QPixmap();
}

} // namespace Utils

#endif // UTILS_H
