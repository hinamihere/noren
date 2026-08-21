#include "dashboard.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <algorithm>

// ---------------------------------------------------------------------------
// ZenMeterWidget: Concentric circles + clay progress arc
// ---------------------------------------------------------------------------
class ZenMeterWidget : public QWidget
{
public:
    explicit ZenMeterWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(130, 130);
    }

    void setPercentage(int percent)
    {
        m_percent = std::clamp(percent, 0, 100);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int size = std::min(width(), height()) - 16;
        const QRectF rect((width() - size) / 2.0, (height() - size) / 2.0, size, size);
        const QPointF center = rect.center();

        // 1. Abstract Zen Concentric Rings
        p.setPen(QPen(QColor(68, 71, 76, 80), 1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(center, size * 0.46, size * 0.46);
        p.drawEllipse(center, size * 0.35, size * 0.35);
        p.drawEllipse(center, size * 0.24, size * 0.24);

        // 2. Active Clay Arc
        if (m_percent > 0) {
            QPen arcPen(QColor(230, 138, 92), 3.5, Qt::SolidLine, Qt::RoundCap);
            p.setPen(arcPen);
            // In Qt, 0 deg is 3 o'clock, so start at 90 deg (12 o'clock)
            const int startAngle = 90 * 16;
            const int spanAngle = -static_cast<int>((m_percent / 100.0) * 360.0 * 16);
            p.drawArc(rect.adjusted(2, 2, -2, -2), startAngle, spanAngle);
        }

        // 3. Center percentage text
        p.setPen(QColor(230, 226, 215));
        QFont font = p.font();
        font.setBold(true);
        font.setPixelSize(std::max(16, size / 5));
        p.setFont(font);

        const QString text = QString::number(m_percent) + "%";
        p.drawText(rect, Qt::AlignCenter, text);
    }

private:
    int m_percent{80};
};

// ---------------------------------------------------------------------------
// CadenceChartWidget: Session timeline bars with background seams
// ---------------------------------------------------------------------------
class CadenceChartWidget : public QWidget
{
public:
    explicit CadenceChartWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(180);
    }

    void setBarValues(const QVector<double> &values)
    {
        m_values = values;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int h = height() - 32; // leave space for bottom axis
        const int w = width();

        // 1. Background Grid Seams
        p.setPen(QPen(QColor(232, 228, 217, 30), 1, Qt::DashLine));
        for (int i = 1; i <= 3; ++i) {
            int y = (h / 4) * i;
            p.drawLine(0, y, w, y);
        }

        // 2. Vertical Clay Bars
        if (m_values.isEmpty()) {
            m_values = {0.15, 0.35, 0.70, 0.90, 0.45, 0.20, 1.0, 0.60};
        }

        const int barCount = m_values.size();
        const double gap = 12.0;
        const double totalGap = gap * (barCount + 1);
        const double barWidth = std::max(8.0, (w - totalGap) / barCount);

        for (int i = 0; i < barCount; ++i) {
            double ratio = std::clamp(m_values[i], 0.05, 1.0);
            double barH = (h - 10) * ratio;
            double x = gap + i * (barWidth + gap);
            double y = h - barH;

            // Gradient intensity based on height
            int alpha = static_cast<int>(80 + ratio * 175);
            QColor barColor(230, 138, 92, alpha);

            p.setPen(Qt::NoPen);
            p.setBrush(barColor);
            p.drawRoundedRect(QRectF(x, y, barWidth, barH), 2, 2);
        }

        // 3. Bottom Axis Labels
        p.setPen(QColor(158, 155, 145));
        QFont font = p.font();
        font.setPixelSize(11);
        font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
        p.setFont(font);

        const QString labels[] = {"8 AM", "12 PM", "4 PM", "8 PM"};
        for (int i = 0; i < 4; ++i) {
            int x = (w / 4) * i;
            p.drawText(QRect(x, h + 8, w / 4, 20), (i == 0) ? Qt::AlignLeft : (i == 3 ? Qt::AlignRight : Qt::AlignCenter), labels[i]);
        }
    }

private:
    QVector<double> m_values{0.15, 0.35, 0.70, 0.90, 0.45, 0.20, 1.0, 0.60};
};

// ---------------------------------------------------------------------------
// AppBreakdownWidget: Horizontal progress breakdown of active applications
// ---------------------------------------------------------------------------
class AppBreakdownWidget : public QWidget
{
public:
    explicit AppBreakdownWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_layout = new QVBoxLayout(this);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(14);
    }

    void updateApps(const QList<AppUsageSummary> &report, qint64 totalSeconds)
    {
        // Clear previous widgets
        QLayoutItem *child;
        while ((child = m_layout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->deleteLater();
            }
            delete child;
        }

        if (report.isEmpty()) {
            auto *noDataLabel = new QLabel("No activity recorded yet.", this);
            noDataLabel->setStyleSheet("color: #9e9b91; font-size: 13px;");
            m_layout->addWidget(noDataLabel);
            return;
        }

        const int maxItems = std::min(static_cast<int>(report.size()), 4);
        for (int i = 0; i < maxItems; ++i) {
            const auto &item = report.at(i);
            const qint64 hours = item.totalSeconds / 3600;
            const qint64 minutes = (item.totalSeconds % 3600) / 60;
            const int percent = (totalSeconds > 0) ? static_cast<int>((item.totalSeconds * 100) / totalSeconds) : 0;

            auto *row = new QWidget(this);
            auto *rowLayout = new QVBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(4);

            auto *headerLayout = new QHBoxLayout();
            headerLayout->setContentsMargins(0, 0, 0, 0);

            auto *nameLabel = new QLabel(item.appId, row);
            nameLabel->setStyleSheet("color: #e6e2d7; font-size: 13px; font-weight: 500;");

            QString timeStr = QString("%1h %2m").arg(hours).arg(minutes, 2, 10, QChar('0'));
            auto *timeLabel = new QLabel(timeStr, row);
            timeLabel->setStyleSheet("color: #9e9b91; font-size: 12px;");

            headerLayout->addWidget(nameLabel);
            headerLayout->addStretch();
            headerLayout->addWidget(timeLabel);

            auto *bar = new QProgressBar(row);
            bar->setRange(0, 100);
            bar->setValue(percent);
            bar->setTextVisible(false);
            bar->setFixedHeight(4);
            bar->setStyleSheet(R"(
                QProgressBar {
                    background-color: #2b2a23;
                    border: none;
                    border-radius: 2px;
                }
                QProgressBar::chunk {
                    background-color: #e68a5c;
                    border-radius: 2px;
                }
            )");

            rowLayout->addLayout(headerLayout);
            rowLayout->addWidget(bar);

            m_layout->addWidget(row);
        }
        m_layout->addStretch();
    }

private:
    QVBoxLayout *m_layout{nullptr};
};

// ---------------------------------------------------------------------------
// Dashboard Implementation
// ---------------------------------------------------------------------------
Dashboard::Dashboard(Database *db, QWidget *parent)
    : QMainWindow(parent)
    , m_db(db)
{
    setWindowTitle("NOREN | 暖簾 — Stats");
    resize(980, 640);
    setMinimumSize(820, 520);

    setupUi();

    if (m_db) {
        connect(m_db, &Database::reportReady, this, &Dashboard::onReportReady);
    }

    m_refreshTimer.setInterval(30000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &Dashboard::refreshReport);
}

void Dashboard::setupUi()
{
    // Apply Global Aizome Theme Stylesheet
    setStyleSheet(R"(
        QMainWindow {
            background-color: #14140d;
        }
        QWidget#centralContainer {
            background-color: #14140d;
        }
        QFrame#sideNav {
            background-color: #14140d;
            border-right: 1px solid rgba(232, 228, 217, 0.12);
        }
        QFrame#panelCard {
            background-color: #212019;
            border: 1px solid rgba(232, 228, 217, 0.12);
            border-radius: 2px;
        }
        QLabel {
            color: #e6e2d7;
            font-family: 'Segoe UI', 'Hanken Grotesk', sans-serif;
        }
    )");

    auto *centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralContainer");
    auto *rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // =========================================================================
    // 1. SideNav (Stats only for v0.1)
    // =========================================================================
    auto *sideNav = new QFrame(this);
    sideNav->setObjectName("sideNav");
    sideNav->setFixedWidth(220);

    auto *sideLayout = new QVBoxLayout(sideNav);
    sideLayout->setContentsMargins(20, 32, 20, 24);
    sideLayout->setSpacing(8);

    // Brand Title
    auto *brandTitle = new QLabel("NOREN | 暖簾", sideNav);
    brandTitle->setStyleSheet("color: #e68a5c; font-size: 15px; font-weight: 700; letter-spacing: 2px;");
    auto *brandSub = new QLabel("DEEP WORK", sideNav);
    brandSub->setStyleSheet("color: #9e9b91; font-size: 11px; font-weight: 600; letter-spacing: 2px; margin-bottom: 24px;");

    sideLayout->addWidget(brandTitle);
    sideLayout->addWidget(brandSub);

    // Active Stats Tab
    auto *statsTab = new QFrame(sideNav);
    statsTab->setStyleSheet(R"(
        QFrame {
            background-color: #2b2a23;
            border-left: 4px solid #ffb5a1;
            border-radius: 0px;
        }
    )");
    auto *tabLayout = new QHBoxLayout(statsTab);
    tabLayout->setContentsMargins(14, 10, 14, 10);

    auto *tabLabel = new QLabel("Stats", statsTab);
    tabLabel->setStyleSheet("color: #ffb5a1; font-size: 14px; font-weight: 500;");
    tabLayout->addWidget(tabLabel);

    sideLayout->addWidget(statsTab);
    sideLayout->addStretch();

    // User / Version Footer
    auto *footer = new QLabel("noren v0.1", sideNav);
    footer->setStyleSheet("color: rgba(230, 226, 215, 0.4); font-size: 11px; letter-spacing: 1px;");
    sideLayout->addWidget(footer);

    rootLayout->addWidget(sideNav);

    // =========================================================================
    // 2. Main Content Bento Grid
    // =========================================================================
    auto *mainArea = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(mainArea);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(20);

    // Top: Hero Card (Total Focus Time)
    auto *heroCard = new QFrame(mainArea);
    heroCard->setObjectName("panelCard");
    heroCard->setMinimumHeight(130);
    auto *heroLayout = new QVBoxLayout(heroCard);
    heroLayout->setContentsMargins(24, 20, 24, 20);
    heroLayout->setSpacing(6);

    auto *heroTag = new QLabel("■  TODAY'S DEEP WORK", heroCard);
    heroTag->setStyleSheet("color: #9e9b91; font-size: 11px; font-weight: 600; letter-spacing: 1.5px;");

    m_heroTimeLabel = new QLabel("0<font size='5' color='#9e9b91'>h</font> 00<font size='5' color='#9e9b91'>m</font>", heroCard);
    m_heroTimeLabel->setStyleSheet("font-size: 42px; font-weight: 300; color: #e6e2d7;");

    m_heroSubtitleLabel = new QLabel("Tracking live window focus", heroCard);
    m_heroSubtitleLabel->setStyleSheet("color: #9e9b91; font-size: 13px;");

    heroLayout->addWidget(heroTag);
    heroLayout->addWidget(m_heroTimeLabel);
    heroLayout->addWidget(m_heroSubtitleLabel);

    mainLayout->addWidget(heroCard);

    // Bottom Grid: Left (Cadence Chart) + Right (Consistency & Breakdown)
    auto *gridWidget = new QWidget(mainArea);
    auto *gridLayout = new QGridLayout(gridWidget);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(20);

    // Middle Left: Session Cadence
    auto *cadenceCard = new QFrame(gridWidget);
    cadenceCard->setObjectName("panelCard");
    auto *cadenceLayout = new QVBoxLayout(cadenceCard);
    cadenceLayout->setContentsMargins(24, 20, 24, 20);

    auto *cadenceTag = new QLabel("SESSION CADENCE", cadenceCard);
    cadenceTag->setStyleSheet("color: #9e9b91; font-size: 11px; font-weight: 600; letter-spacing: 1.5px; margin-bottom: 8px;");
    m_cadenceWidget = new CadenceChartWidget(cadenceCard);

    cadenceLayout->addWidget(cadenceTag);
    cadenceLayout->addWidget(m_cadenceWidget);

    gridLayout->addWidget(cadenceCard, 0, 0, 2, 1);

    // Middle Right Top: Consistency
    auto *zenCard = new QFrame(gridWidget);
    zenCard->setObjectName("panelCard");
    auto *zenLayout = new QVBoxLayout(zenCard);
    zenLayout->setContentsMargins(24, 16, 24, 16);

    auto *zenTag = new QLabel("CONSISTENCY", zenCard);
    zenTag->setStyleSheet("color: #9e9b91; font-size: 11px; font-weight: 600; letter-spacing: 1.5px;");
    m_zenWidget = new ZenMeterWidget(zenCard);
    auto *zenSub = new QLabel("Active Session", zenCard);
    zenSub->setAlignment(Qt::AlignCenter);
    zenSub->setStyleSheet("color: #9e9b91; font-size: 12px;");

    zenLayout->addWidget(zenTag);
    zenLayout->addWidget(m_zenWidget, 0, Qt::AlignCenter);
    zenLayout->addWidget(zenSub);

    gridLayout->addWidget(zenCard, 0, 1);

    // Middle Right Bottom: App Breakdown
    auto *breakdownCard = new QFrame(gridWidget);
    breakdownCard->setObjectName("panelCard");
    auto *breakdownLayout = new QVBoxLayout(breakdownCard);
    breakdownLayout->setContentsMargins(24, 16, 24, 16);

    auto *breakdownTag = new QLabel("TOP APPLICATIONS", breakdownCard);
    breakdownTag->setStyleSheet("color: #9e9b91; font-size: 11px; font-weight: 600; letter-spacing: 1.5px; margin-bottom: 6px;");
    m_breakdownWidget = new AppBreakdownWidget(breakdownCard);

    breakdownLayout->addWidget(breakdownTag);
    breakdownLayout->addWidget(m_breakdownWidget);

    gridLayout->addWidget(breakdownCard, 1, 1);

    gridLayout->setColumnStretch(0, 3);
    gridLayout->setColumnStretch(1, 2);

    mainLayout->addWidget(gridWidget, 1);

    rootLayout->addWidget(mainArea, 1);
    setCentralWidget(centralWidget);
}

void Dashboard::refreshReport()
{
    if (m_db) {
        m_db->requestReportForToday();
    }
}

void Dashboard::onReportReady(const QList<AppUsageSummary> &report)
{
    qint64 totalSeconds = 0;
    for (const auto &item : report) {
        totalSeconds += item.totalSeconds;
    }

    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;

    m_heroTimeLabel->setText(
        QString("%1<font size='5' color='#9e9b91'>h</font> %2<font size='5' color='#9e9b91'>m</font>")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
    );

    if (report.isEmpty()) {
        m_heroSubtitleLabel->setText("No tracked window activity recorded today yet.");
    } else {
        m_heroSubtitleLabel->setText(QString("%1 application(s) actively recorded today").arg(report.size()));
    }

    // Update Breakdown
    if (m_breakdownWidget) {
        m_breakdownWidget->updateApps(report, totalSeconds);
    }

    // Update Zen consistency meter based on focus target (e.g. 4h target)
    if (m_zenWidget) {
        const double targetSeconds = 4 * 3600.0;
        int score = static_cast<int>((totalSeconds / targetSeconds) * 100);
        m_zenWidget->setPercentage(std::clamp(score, 10, 100));
    }
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