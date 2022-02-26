#include <QScrollBar>
#include <QPainter>
#include <QPaintEvent>
#include <QDateTime>
#include <QToolTip>

#include "calendar_heatmap_widget.h"

constexpr int MARGIN_LEFT = 10;
constexpr int MARGIN_RIGHT = 10;
constexpr int MARGIN_TOP = 10;
constexpr int NUM_WEEKS = 54;
constexpr int GAP = 2;
//constexpr int YEAR_GAP = 20;
constexpr int DAYS_IN_WEEK = 7;
constexpr int DAY_OF_WEEK_COL_WIDTH = 50;
constexpr int GRADE_COLOR_MAP[] = {0, 1, 2, 3, 4, 4};
constexpr QColor GRADE_COLORS[] =
{
    QColor(235, 237, 240),
    QColor(0x9b, 0xe9, 0xa8),
    QColor(0x40, 0xc4, 0x63),
    QColor(0x30, 0xa1, 0x4e),
    QColor(0x21, 0x6e, 0x39),
};
const char* DAY_NAMES[] = { "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su" };
const char* MONTH_NAMES[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

CalendarHeatmapWidget::CalendarHeatmapWidget(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    setAttribute(Qt::WA_AlwaysShowToolTips);
    setMouseTracking(true);
    resize(1000, 1000);
    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
}

void CalendarHeatmapWidget::set_data(QDate first_day, std::vector<int> d)
{
    m_data_first_day = first_day;
    m_data = std::move(d);
    auto max_it = std::max_element(m_data.begin(), m_data.end());
    m_max_value = max_it != m_data.end() ? *max_it : 0;

    if (m_data_first_day.isValid())
    {
        if (m_data_first_day.year() >= 1900 && m_data_first_day.year() <= 2100)
            m_is_data_valid = !m_data.empty();
    }

    int today_year = QDateTime::currentDateTime().date().year();
    if (m_is_data_valid)
    {
        int first_day_data_year = m_data_first_day.year();
        m_min_year = std::min(first_day_data_year, today_year);
        m_max_year = std::max(first_day_data_year, today_year);
    }
    else
    {
        m_min_year = today_year;
        m_max_year = today_year;
    }
    viewport()->repaint();
}

void CalendarHeatmapWidget::showEvent(QShowEvent* event)
{
    if (verticalScrollBar()->isVisible())
    {
        m_layout.scrollbar_width = verticalScrollBar()->width();
        setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    }

    if (horizontalScrollBar()->isVisible())
    {
        m_layout.scrollbar_height = horizontalScrollBar()->height();
        setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    }
}

void CalendarHeatmapWidget::resizeEvent(QResizeEvent*)
{
    int num_years = m_is_data_valid ? (m_max_year - m_min_year + 1) : 1;

    m_layout.internal_width = width() > 500 ? width() : 500;
    m_layout.content_width = m_layout.internal_width - MARGIN_LEFT - MARGIN_RIGHT - m_layout.scrollbar_width;
    m_layout.left_column_width = m_layout.content_width / 50;
    m_layout.heatmap_width = m_layout.content_width - m_layout.left_column_width;
    m_layout.heatmap_top = MARGIN_TOP + m_layout.year_gap;
    m_layout.weak_width = m_layout.heatmap_width / NUM_WEEKS;
    m_layout.left_pad = (m_layout.heatmap_width - m_layout.weak_width * NUM_WEEKS) / 2;
    m_layout.content_left = MARGIN_LEFT + m_layout.left_pad;
    m_layout.heatmap_left = m_layout.content_left + m_layout.left_column_width;
    m_layout.square_size = m_layout.weak_width - GAP;
    m_layout.year_gap = m_layout.weak_width;
    m_layout.year_interval_height = 7 * m_layout.weak_width + m_layout.year_gap;
    m_layout.weak_height = 7 * m_layout.weak_width;

    m_layout.internal_height =
        MARGIN_TOP +
        num_years * (7 * m_layout.weak_width + m_layout.year_gap) +
        m_layout.scrollbar_height;

    verticalScrollBar()->setSingleStep(10);
    verticalScrollBar()->setPageStep(height());
    if (m_layout.internal_height > height())
    {
        verticalScrollBar()->setRange(0, m_layout.internal_height - height());
    }
    else
    {
        verticalScrollBar()->setRange(0, 0);
    }

    horizontalScrollBar()->setSingleStep(10);
    horizontalScrollBar()->setPageStep(width());
    if (m_layout.internal_width > width())
    {
        horizontalScrollBar()->setRange(0, m_layout.internal_width - width());
    }
    else
    {
        horizontalScrollBar()->setRange(0, 0);
    }
}

void CalendarHeatmapWidget::paintEvent(QPaintEvent*)
{
    QDate today = QDateTime::currentDateTime().date();
    int today_year = today.year();
    int today_day_index = today.dayOfYear() - 1;

    QPainter painter(viewport());
    painter.translate(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
    QFont font = painter.font();

    int year_index = 0;
    for (int year = m_max_year; year >= m_min_year; --year, ++year_index)
    {
        font.setPixelSize(m_layout.weak_width + 2);
        painter.setFont(font);

        painter.save();
        int year_top = MARGIN_TOP + year_index * m_layout.year_interval_height;
        painter.translate(m_layout.content_left - 5, year_top + m_layout.year_interval_height);
        painter.rotate(-90);
        QRect year_text_rect(0, 0, m_layout.weak_height, m_layout.left_column_width);
        painter.drawText(year_text_rect, Qt::AlignCenter, QString::number(year));
        painter.restore();

        QDate jan1(year, 1, 1);
        QDate dec31(year, 12, 31);

        int jan1_day_of_week = jan1.dayOfWeek();
        int dec31_day_of_week = dec31.dayOfWeek();
        int days_in_year = jan1.daysInYear();
        int last_week = (days_in_year + jan1_day_of_week - 1) / 7;

        int day_of_year = 0;

        for (int w = 0; w <= last_week; w++)
        {
            int x = m_layout.heatmap_left + w * m_layout.weak_width;

            font.setPixelSize(m_layout.weak_width - 2);
            painter.setFont(font);

            QDate last_day_of_week = jan1.addDays(w * 7 - jan1_day_of_week + 7);
            if (last_day_of_week.day() <= 7 && w != last_week)
            {
                QRect month_text_rect(x, year_top, m_layout.weak_width * 4, m_layout.year_gap);
                painter.drawText(month_text_rect, Qt::AlignBottom, MONTH_NAMES[last_day_of_week.month() - 1]);
            }

            int week_start = (w == 0) ? (jan1_day_of_week - 1) : 0;
            int week_end = (w == last_week) ? (dec31_day_of_week - 1) : 6;

            for (int wd = week_start; wd <= week_end; wd++)
            {
                int count = get_count(year, day_of_year);

                int y = MARGIN_TOP + year_index * (7 * m_layout.weak_width + m_layout.year_gap) + m_layout.year_gap + wd * m_layout.weak_width;
                int grade = 0;
                if (count > 0 && m_max_value > 0)
                    grade = (count - 1) * 5 / m_max_value + 1;
                QColor color = GRADE_COLORS[GRADE_COLOR_MAP[grade]];
                painter.fillRect(x, y, m_layout.square_size, m_layout.square_size, color);

                if (year == today_year && day_of_year == today_day_index)
                    painter.drawRect(x - 1, y - 1, m_layout.square_size + 1, m_layout.square_size + 1);

                ++day_of_year;
            }
        }
    }

}

void CalendarHeatmapWidget::mouseMoveEvent(QMouseEvent* event)
{
    QPoint screen_pos = mapToGlobal(event->pos());
    screen_pos.setX(screen_pos.x() + 10);
    int x = event->pos().x() + horizontalScrollBar()->value();
    int y = event->pos().y() + verticalScrollBar()->value();
    x -= m_layout.heatmap_left;
    y -= m_layout.heatmap_top;
    QString tooltip_text;
    if (x >= 0 && y >= 0)
    {
        int day_of_week = (y % m_layout.year_interval_height) / m_layout.weak_width;
        if (day_of_week < 7)
        {
            int year_index = y / m_layout.year_interval_height;
            int year = m_max_year - year_index;
            if (year >= m_min_year && year >= m_max_year)
            {
                QDate jan1(year, 1, 1);
                int jan1_day_of_week = jan1.dayOfWeek();
                int week_index = x / m_layout.weak_width;
                int day_index = week_index * DAYS_IN_WEEK - jan1_day_of_week + day_of_week + 1;
                if (day_index >= 0 && day_index < jan1.daysInYear())
                {
                    QDate selected = jan1.addDays(day_index);
                    tooltip_text = selected.toString();
                    tooltip_text += "\n" + QString::number(get_count(year, day_index));
                }
            }
        }
    }

    if (!tooltip_text.isEmpty())
        QToolTip::showText(screen_pos, tooltip_text);
    else
        QToolTip::hideText();
}

int CalendarHeatmapWidget::get_count(int year, int day_index) const
{
    int res = 0;
    if (m_is_data_valid)
    {
        QDate jan1(year, 1, 1);
        QDate base_date = QDate(m_min_year, 1, 1);
        int jan1_day_from_base = base_date.daysTo(jan1);
        int data_first_day_from_base = base_date.daysTo(m_data_first_day);
        int data_day = jan1_day_from_base + day_index - data_first_day_from_base;
        if (data_day >= 0 && data_day < m_data.size())
            res = m_data[data_day];
    }
    return res;
}
