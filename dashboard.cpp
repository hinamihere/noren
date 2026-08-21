#include "dashboard.h"
#include <QLabel>

Dashboard::Dashboard(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("noren");
    resize(600, 400);

    auto *label = new QLabel("No data yet. Tracking will appear here.");
    label->setAlignment(Qt::AlignCenter);
    setCentralWidget(label);
}

void Dashboard::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();
}