#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QHideEvent>
#include <QShowEvent>
#include <QTimer>
#include "database.h"

class QTableWidget;
class QLabel;

class Dashboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit Dashboard(Database *db = nullptr, QWidget *parent = nullptr);

public slots:
    void refreshReport();

private slots:
    void onReportReady(const QList<AppUsageSummary> &report);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Database *m_db{nullptr};
    QTableWidget *m_tableWidget{nullptr};
    QLabel *m_totalLabel{nullptr};
    QTimer m_refreshTimer;
};

#endif // DASHBOARD_H
