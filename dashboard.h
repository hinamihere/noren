#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QMainWindow>
#include <QCloseEvent>

class Dashboard : public QMainWindow
{
    Q_OBJECT
public:
    explicit Dashboard(QWidget *parent = nullptr);
private:
    void closeEvent(QCloseEvent *event) override;
};

#endif // DASHBOARD_H
