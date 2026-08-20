#ifndef NORENTRAY_H
#define NORENTRAY_H

#include <QObject>

class NorenTray : public QObject
{
    Q_OBJECT

public:
    explicit NorenTray(QObject *parent = nullptr);
    ~NorenTray() override;
};
#endif // NORENTRAY_H
