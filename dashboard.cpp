#include "dashboard.h"

#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

Dashboard::Dashboard(Database *db, QWidget *parent)
    : QMainWindow(parent)
    , m_db(db)
{
    setWindowTitle("noren — Dashboard");
    resize(600, 400);

    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(2);
    m_tableWidget->setHorizontalHeaderLabels({"Application", "Time"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->verticalHeader()->setVisible(false);

    layout->addWidget(m_tableWidget);

    auto *bottomLayout = new QHBoxLayout();
    m_totalLabel = new QLabel("Total: 0h 00m", this);
    QFont font = m_totalLabel->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 1);
    m_totalLabel->setFont(font);

    bottomLayout->addStretch();
    bottomLayout->addWidget(m_totalLabel);
    layout->addLayout(bottomLayout);

    setCentralWidget(centralWidget);

    if (m_db) {
        connect(m_db, &Database::reportReady, this, &Dashboard::onReportReady);
    }

    m_refreshTimer.setInterval(30000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &Dashboard::refreshReport);
}

void Dashboard::refreshReport()
{
    if (m_db) {
        m_db->requestReportForToday();
    }
}

void Dashboard::onReportReady(const QList<AppUsageSummary> &report)
{
    m_tableWidget->setRowCount(report.size());

    qint64 totalAllSeconds = 0;
    for (int i = 0; i < report.size(); ++i) {
        const auto &item = report.at(i);
        totalAllSeconds += item.totalSeconds;

        const qint64 hours = item.totalSeconds / 3600;
        const qint64 minutes = (item.totalSeconds % 3600) / 60;

        auto *appItem = new QTableWidgetItem(item.appId);
        auto *timeItem = new QTableWidgetItem(QString("%1h %2m").arg(hours).arg(minutes, 2, 10, QChar('0')));
        timeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_tableWidget->setItem(i, 0, appItem);
        m_tableWidget->setItem(i, 1, timeItem);
    }

    const qint64 totalHours = totalAllSeconds / 3600;
    const qint64 totalMinutes = (totalAllSeconds % 3600) / 60;
    m_totalLabel->setText(QString("Total: %1h %2m").arg(totalHours).arg(totalMinutes, 2, 10, QChar('0')));
}

void Dashboard::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    refreshReport();
    m_refreshTimer.start();
}

void Dashboard::hideEvent(QHideEvent *event)
{
    QMainWindow::hideEvent(event);
    m_refreshTimer.stop();
}

void Dashboard::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}