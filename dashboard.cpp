#include "dashboard.h"

#include <QCoreApplication>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QProgressBar>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QVBoxLayout>
#include <algorithm>

// ---------------------------------------------------------------------------
// ZenMeterWidget: Concentric circles + clay progress arc for Primary Focus %
// ---------------------------------------------------------------------------
class ZenMeterWidget : public QWidget
{
public:
    explicit ZenMeterWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(80, 80);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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

        const int size = std::min(width(), height()) - 10;
        if (size <= 10) return;

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
            QPen arcPen(QColor(230, 138, 92), 3.0, Qt::SolidLine, Qt::RoundCap);
            p.setPen(arcPen);
            const int startAngle = 90 * 16;
            const int spanAngle = -static_cast<int>((m_percent / 100.0) * 360.0 * 16);
            p.drawArc(rect.adjusted(2, 2, -2, -2), startAngle, spanAngle);
        }

        // 3. Center percentage text
        p.setPen(QColor(230, 226, 215));
        QFont font = p.font();
        font.setBold(true);
        font.setPixelSize(std::max(12, size / 5));
        p.setFont(font);

        const QString text = QString::number(m_percent) + "%";
        p.drawText(rect, Qt::AlignCenter, text);
    }

private:
    int m_percent{0};
};

#include <QEvent>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QToolTip>

// ---------------------------------------------------------------------------
// CadenceChartWidget: 7-Day Real Weekly Focus Bar Chart with Interactive Tooltips
// ---------------------------------------------------------------------------
class CadenceChartWidget : public QWidget
{
public:
    explicit CadenceChartWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(110);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMouseTracking(true);
    }

    void setWeeklyData(const QList<DayUsageSummary> &weeklyData)
    {
        m_weeklyData = weeklyData;
        update();
    }

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::ToolTip) {
            auto *helpEvent = static_cast<QHelpEvent *>(event);
            const int idx = barIndexAt(helpEvent->pos());
            if (idx >= 0 && idx < m_weeklyData.size()) {
                const auto &day = m_weeklyData.at(idx);
                const qint64 totalSecs = day.totalSeconds;
                const qint64 hours = totalSecs / 3600;
                const qint64 minutes = (totalSecs % 3600) / 60;

                QString timeText;
                if (hours > 0) {
                    timeText = QString("%1h %2m").arg(hours).arg(minutes, 2, 10, QChar('0'));
                } else if (minutes > 0) {
                    timeText = QString("%1m").arg(minutes);
                } else {
                    timeText = "0m";
                }

                const QString tip = QString("<b>%1</b>: %2").arg(day.dayLabel, timeText);
                QToolTip::showText(helpEvent->globalPos(), tip, this);
                return true;
            }
            QToolTip::hideText();
            event->ignore();
            return true;
        }
        return QWidget::event(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        const int idx = barIndexAt(event->pos());
        if (idx != m_hoveredIndex) {
            m_hoveredIndex = idx;
            update();
        }
        QWidget::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        if (m_hoveredIndex != -1) {
            m_hoveredIndex = -1;
            update();
        }
        QWidget::leaveEvent(event);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int h = height() - 24; // leave space for day labels
        const int w = width();
        if (h <= 10 || w <= 20) return;

        // 1. Background Grid Seam lines
        p.setPen(QPen(QColor(232, 228, 217, 30), 1, Qt::DashLine));
        for (int i = 1; i <= 3; ++i) {
            int y = (h / 4) * i;
            p.drawLine(0, y, w, y);
        }

        if (m_weeklyData.isEmpty()) {
            p.setPen(QColor(158, 155, 145));
            p.drawText(QRect(0, 0, w, h), Qt::AlignCenter, "No activity recorded yet");
            return;
        }

        qint64 maxSecs = 1800; // minimum 30m scale
        for (const auto &day : m_weeklyData) {
            if (day.totalSeconds > maxSecs) {
                maxSecs = day.totalSeconds;
            }
        }

        const int dayCount = m_weeklyData.size();
        const double gap = std::clamp(w / 40.0, 4.0, 14.0);
        const double totalGap = gap * (dayCount + 1);
        const double barWidth = std::max(6.0, (w - totalGap) / dayCount);

        for (int i = 0; i < dayCount; ++i) {
            const auto &day = m_weeklyData.at(i);
            const double x = gap + i * (barWidth + gap);
            const bool isHovered = (i == m_hoveredIndex);

            if (day.totalSeconds > 0) {
                const double ratio = std::clamp(static_cast<double>(day.totalSeconds) / maxSecs, 0.06, 1.0);
                const double barH = (h - 8) * ratio;
                const double y = h - barH;

                int alpha = static_cast<int>(120 + ratio * 135);
                if (isHovered) {
                    alpha = std::min(240, alpha + 30);
                }
                const QColor barColor(230, 138, 92, alpha);

                p.setPen(Qt::NoPen);
                p.setBrush(barColor);
                p.drawRoundedRect(QRectF(x, y, barWidth, barH), 2, 2);
            } else {
                p.setPen(QPen(isHovered ? QColor(230, 138, 92, 140) : QColor(232, 228, 217, 40), 1.5));
                p.drawLine(QPointF(x, h - 1), QPointF(x + barWidth, h - 1));
            }

            // Day label below bar
            p.setPen(day.dayLabel == "Today" || isHovered ? QColor(230, 138, 92) : QColor(158, 155, 145));
            QFont font = p.font();
            font.setBold(day.dayLabel == "Today" || isHovered);
            font.setPixelSize(10);
            p.setFont(font);

            p.drawText(QRectF(x - 6, h + 4, barWidth + 12, 18), Qt::AlignCenter, day.dayLabel);
        }
    }

private:
    int barIndexAt(const QPoint &pos) const
    {
        if (m_weeklyData.isEmpty()) return -1;

        const int h = height() - 24;
        const int w = width();
        if (pos.y() < 0 || pos.y() > height()) return -1;

        const int dayCount = m_weeklyData.size();
        const double gap = std::clamp(w / 40.0, 4.0, 14.0);
        const double totalGap = gap * (dayCount + 1);
        const double barWidth = std::max(6.0, (w - totalGap) / dayCount);

        for (int i = 0; i < dayCount; ++i) {
            const double x = gap + i * (barWidth + gap);
            // Allow clicking/hovering near the column
            if (pos.x() >= x - (gap / 2.0) && pos.x() <= x + barWidth + (gap / 2.0)) {
                return i;
            }
        }
        return -1;
    }

    QList<DayUsageSummary> m_weeklyData;
    int m_hoveredIndex{-1};
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
        m_layout->setSpacing(8);
    }

    void updateApps(const QList<AppUsageSummary> &report, qint64 totalSeconds)
    {
        QLayoutItem *child;
        while ((child = m_layout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->deleteLater();
            }
            delete child;
        }

        if (report.isEmpty()) {
            auto *noDataLabel = new QLabel("No activity recorded yet.", this);
            noDataLabel->setStyleSheet("color: #9e9b91; font-size: 11px;");
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
            rowLayout->setSpacing(2);

            auto *headerLayout = new QHBoxLayout();
            headerLayout->setContentsMargins(0, 0, 0, 0);

            auto *nameLabel = new QLabel(item.appId, row);
            nameLabel->setStyleSheet("color: #e6e2d7; font-size: 11px; font-weight: 500;");

            QString timeStr = QString("%1h %2m").arg(hours).arg(minutes, 2, 10, QChar('0'));
            auto *timeLabel = new QLabel(timeStr, row);
            timeLabel->setStyleSheet("color: #9e9b91; font-size: 10px;");

            headerLayout->addWidget(nameLabel);
            headerLayout->addStretch();
            headerLayout->addWidget(timeLabel);

            auto *bar = new QProgressBar(row);
            bar->setRange(0, 100);
            bar->setValue(percent);
            bar->setTextVisible(false);
            bar->setFixedHeight(3);
            bar->setStyleSheet(R"(
                QProgressBar {
                    background-color: #2b2a23;
                    border: none;
                    border-radius: 1px;
                }
                QProgressBar::chunk {
                    background-color: #e68a5c;
                    border-radius: 1px;
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

#include "utils.h"

// ---------------------------------------------------------------------------
// Dashboard Implementation
// ---------------------------------------------------------------------------
Dashboard::Dashboard(Database *db, QWidget *parent)
    : QMainWindow(parent)
    , m_db(db)
{
    setWindowTitle("NOREN | 暖簾 — Stats");

    const QIcon appIcon = Utils::loadIcon("noren.jpg");
    if (!appIcon.isNull()) {
        setWindowIcon(appIcon);
    }

    resize(720, 480);
    setMinimumSize(560, 360);

    setupUi();

    if (m_db) {
        connect(m_db, &Database::dashboardDataReady, this, &Dashboard::onDashboardDataReady);
    }

    m_refreshTimer.setInterval(30000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &Dashboard::refreshReport);
}

void Dashboard::setupUi()
{
    // Apply Global Aizome Theme Stylesheet with smooth scrollbars
    setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #14140d;
            color: #e6e2d7;
        }
        QWidget#centralContainer, QWidget#mainArea, QScrollArea, QAbstractScrollArea, QAbstractScrollArea::viewport {
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
        QFrame#statsTab {
            background-color: #2b2a23;
            border-left: 3px solid #ffb5a1;
            border-radius: 0px;
        }
        QLabel {
            color: #e6e2d7;
            font-family: 'Segoe UI', 'Hanken Grotesk', sans-serif;
            background-color: transparent;
        }
        QScrollArea {
            background-color: #14140d;
            border: none;
        }
        QScrollBar:vertical {
            background: #14140d;
            width: 6px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #2b2a23;
            min-height: 20px;
            border-radius: 3px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QToolTip {
            background-color: #212019;
            color: #e6e2d7;
            border: 1px solid rgba(232, 228, 217, 0.2);
            padding: 5px 8px;
            font-family: 'Segoe UI', 'Hanken Grotesk', sans-serif;
            font-size: 11px;
            border-radius: 2px;
        }
    )");

    auto *centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralContainer");
    auto *rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // =========================================================================
    // 1. SideNav (Stats only for v0.1) - Compact & Responsive width
    // =========================================================================
    auto *sideNav = new QFrame(this);
    sideNav->setObjectName("sideNav");
    sideNav->setFixedWidth(160);

    auto *sideLayout = new QVBoxLayout(sideNav);
    sideLayout->setContentsMargins(14, 20, 14, 16);
    sideLayout->setSpacing(6);

    // Optional Logo Pixmap
    const QPixmap logoPix = Utils::loadPixmap("noren.jpg");
    if (!logoPix.isNull()) {
        auto *logoLabel = new QLabel(sideNav);
        logoLabel->setPixmap(logoPix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        logoLabel->setStyleSheet("margin-bottom: 2px;");
        sideLayout->addWidget(logoLabel);
    }

    // Brand Title
    auto *brandTitle = new QLabel("NOREN | 暖簾", sideNav);
    brandTitle->setStyleSheet("color: #e68a5c; font-size: 13px; font-weight: 700; letter-spacing: 1.5px;");
    auto *brandSub = new QLabel("DEEP WORK", sideNav);
    brandSub->setStyleSheet("color: #9e9b91; font-size: 10px; font-weight: 600; letter-spacing: 1.5px; margin-bottom: 16px;");

    sideLayout->addWidget(brandTitle);
    sideLayout->addWidget(brandSub);

    // Active Stats Tab
    auto *statsTab = new QFrame(sideNav);
    statsTab->setObjectName("statsTab");
    statsTab->setStyleSheet(R"(
        QFrame#statsTab {
            background-color: #2b2a23;
            border-left: 3px solid #ffb5a1;
            border-radius: 0px;
        }
    )");
    auto *tabLayout = new QHBoxLayout(statsTab);
    tabLayout->setContentsMargins(10, 8, 10, 8);

    auto *tabLabel = new QLabel("Stats", statsTab);
    tabLabel->setStyleSheet("color: #ffb5a1; font-size: 13px; font-weight: 500;");
    tabLayout->addWidget(tabLabel);

    sideLayout->addWidget(statsTab);
    sideLayout->addStretch();

    // User / Version Footer
    auto *footer = new QLabel("noren v0.1", sideNav);
    footer->setStyleSheet("color: rgba(230, 226, 215, 0.4); font-size: 10px; letter-spacing: 1px;");
    sideLayout->addWidget(footer);

    rootLayout->addWidget(sideNav);

    // =========================================================================
    // 2. Main Content Bento Grid inside ScrollArea for perfect responsiveness
    // =========================================================================
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->viewport()->setStyleSheet("background-color: #14140d;");
    scrollArea->setStyleSheet("background-color: #14140d; border: none;");

    auto *mainArea = new QWidget(scrollArea);
    mainArea->setObjectName("mainArea");
    mainArea->setStyleSheet("background-color: #14140d;");
    auto *mainLayout = new QVBoxLayout(mainArea);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Top: Hero Card (Total Recorded Screen Time Today)
    auto *heroCard = new QFrame(mainArea);
    heroCard->setObjectName("panelCard");
    auto *heroLayout = new QVBoxLayout(heroCard);
    heroLayout->setContentsMargins(18, 14, 18, 14);
    heroLayout->setSpacing(4);

    auto *heroTag = new QLabel("■  TODAY'S SCREEN TIME", heroCard);
    heroTag->setStyleSheet("color: #9e9b91; font-size: 10px; font-weight: 600; letter-spacing: 1.2px;");

    m_heroTimeLabel = new QLabel("0<font size='4' color='#9e9b91'>h</font> 00<font size='4' color='#9e9b91'>m</font>", heroCard);
    m_heroTimeLabel->setStyleSheet("font-size: 32px; font-weight: 300; color: #e6e2d7;");

    m_heroSubtitleLabel = new QLabel("Tracking live window focus", heroCard);
    m_heroSubtitleLabel->setStyleSheet("color: #9e9b91; font-size: 11px;");

    heroLayout->addWidget(heroTag);
    heroLayout->addWidget(m_heroTimeLabel);
    heroLayout->addWidget(m_heroSubtitleLabel);

    mainLayout->addWidget(heroCard);

    // Bottom Grid: Left (Weekly Activity) + Right (Primary Focus & Breakdown)
    auto *gridWidget = new QWidget(mainArea);
    auto *gridLayout = new QGridLayout(gridWidget);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(12);

    // Middle Left: Weekly Activity Chart (Real 7-day data)
    auto *cadenceCard = new QFrame(gridWidget);
    cadenceCard->setObjectName("panelCard");
    auto *cadenceLayout = new QVBoxLayout(cadenceCard);
    cadenceLayout->setContentsMargins(16, 12, 16, 12);
    cadenceLayout->setSpacing(6);

    auto *cadenceTag = new QLabel("WEEKLY ACTIVITY", cadenceCard);
    cadenceTag->setStyleSheet("color: #9e9b91; font-size: 10px; font-weight: 600; letter-spacing: 1.2px;");
    m_cadenceWidget = new CadenceChartWidget(cadenceCard);

    cadenceLayout->addWidget(cadenceTag);
    cadenceLayout->addWidget(m_cadenceWidget);

    gridLayout->addWidget(cadenceCard, 0, 0, 2, 1);

    // Middle Right Top: Primary Focus (% of today spent on top app)
    auto *zenCard = new QFrame(gridWidget);
    zenCard->setObjectName("panelCard");
    auto *zenLayout = new QVBoxLayout(zenCard);
    zenLayout->setContentsMargins(14, 10, 14, 10);
    zenLayout->setSpacing(4);

    auto *zenTag = new QLabel("PRIMARY FOCUS", zenCard);
    zenTag->setStyleSheet("color: #9e9b91; font-size: 10px; font-weight: 600; letter-spacing: 1.2px;");
    m_zenWidget = new ZenMeterWidget(zenCard);
    m_zenSubtitleLabel = new QLabel("No activity today", zenCard);
    m_zenSubtitleLabel->setAlignment(Qt::AlignCenter);
    m_zenSubtitleLabel->setStyleSheet("color: #9e9b91; font-size: 11px;");

    zenLayout->addWidget(zenTag);
    zenLayout->addWidget(m_zenWidget, 0, Qt::AlignCenter);
    zenLayout->addWidget(m_zenSubtitleLabel);

    gridLayout->addWidget(zenCard, 0, 1);

    // Middle Right Bottom: Top Applications Breakdown
    auto *breakdownCard = new QFrame(gridWidget);
    breakdownCard->setObjectName("panelCard");
    auto *breakdownLayout = new QVBoxLayout(breakdownCard);
    breakdownLayout->setContentsMargins(14, 10, 14, 10);
    breakdownLayout->setSpacing(4);

    auto *breakdownTag = new QLabel("TOP APPLICATIONS", breakdownCard);
    breakdownTag->setStyleSheet("color: #9e9b91; font-size: 10px; font-weight: 600; letter-spacing: 1.2px;");
    m_breakdownWidget = new AppBreakdownWidget(breakdownCard);

    breakdownLayout->addWidget(breakdownTag);
    breakdownLayout->addWidget(m_breakdownWidget);

    gridLayout->addWidget(breakdownCard, 1, 1);

    gridLayout->setColumnStretch(0, 5);
    gridLayout->setColumnStretch(1, 4);

    mainLayout->addWidget(gridWidget, 1);

    scrollArea->setWidget(mainArea);
    rootLayout->addWidget(scrollArea, 1);

    setCentralWidget(centralWidget);
}

void Dashboard::refreshReport()
{
    if (m_db) {
        m_db->requestDashboardData();
    }
}

void Dashboard::onDashboardDataReady(const DashboardData &data)
{
    // 1. Hero Card: Total Recorded Screen Time Today
    const qint64 totalSeconds = data.todayTotalSeconds;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;

    m_heroTimeLabel->setText(
        QString("%1<font size='4' color='#9e9b91'>h</font> %2<font size='4' color='#9e9b91'>m</font>")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
    );

    if (data.todayApps.isEmpty()) {
        m_heroSubtitleLabel->setText("No tracked window activity recorded today yet.");
    } else {
        m_heroSubtitleLabel->setText(QString("Total active window time across %1 application(s) today").arg(data.todayApps.size()));
    }

    // 2. Weekly Activity Chart (Real 7-day data)
    if (m_cadenceWidget) {
        m_cadenceWidget->setWeeklyData(data.weeklyUsage);
    }

    // 3. Primary Focus Zen Meter (Top App % of today)
    if (m_zenWidget && m_zenSubtitleLabel) {
        if (!data.todayApps.isEmpty() && totalSeconds > 0) {
            const auto &topApp = data.todayApps.first();
            const int percent = static_cast<int>((topApp.totalSeconds * 100) / totalSeconds);
            m_zenWidget->setPercentage(percent);

            const qint64 topHours = topApp.totalSeconds / 3600;
            const qint64 topMins = (topApp.totalSeconds % 3600) / 60;
            m_zenSubtitleLabel->setText(QString("%1 (%2h %3m)").arg(topApp.appId).arg(topHours).arg(topMins, 2, 10, QChar('0')));
        } else {
            m_zenWidget->setPercentage(0);
            m_zenSubtitleLabel->setText("No activity today");
        }
    }

    // 4. Top Applications Breakdown
    if (m_breakdownWidget) {
        m_breakdownWidget->updateApps(data.todayApps, totalSeconds);
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