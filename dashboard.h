#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QHideEvent>
#include <QShowEvent>
#include <QTimer>
#include "database.h"

class QLabel;
class QVBoxLayout;
class CadenceChartWidget;
class ZenMeterWidget;
class AppBreakdownWidget;

class Dashboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit Dashboard(Database *db = nullptr, QWidget *parent = nullptr);

public slots:
    void refreshReport();

private slots:
    void onDashboardDataReady(const DashboardData &data);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Database *m_db{nullptr};
    QTimer m_refreshTimer;

    // UI Widgets
    QLabel *m_heroTimeLabel{nullptr};
    QLabel *m_heroSubtitleLabel{nullptr};
    CadenceChartWidget *m_cadenceWidget{nullptr};
    ZenMeterWidget *m_zenWidget{nullptr};
    QLabel *m_zenSubtitleLabel{nullptr};
    AppBreakdownWidget *m_breakdownWidget{nullptr};

    void setupUi();
};

#endif // DASHBOARD_H
