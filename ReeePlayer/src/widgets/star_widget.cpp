#include <pch.h>
#include <star_widget.h>

constexpr int PaintingScaleFactor = 30;

StarWidget::StarWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAutoFillBackground(true);
    setAttribute(Qt::WA_AlwaysShowToolTips);

    constexpr float pi = 3.14159265f;
    constexpr float alpha = (2 * pi) / 10;
    constexpr float radius = .4f;
    constexpr float inner_radius = radius * .5f;

    QPointF c(0.5, 0.5);
    for (int i = 0; i < 11; ++i)
    {
        float r = (i % 2 == 0) ? inner_radius : radius;
        float omega = alpha * i;
        star_polygon << QPointF((r * std::sin(omega)), (r * std::cos(omega))) + c;
    }

    diamond_polygon << QPointF(0.4, 0.5) << QPointF(0.5, 0.4)
        << QPointF(0.6, 0.5) << QPointF(0.5, 0.6)
        << QPointF(0.4, 0.5);

    m_ratings.resize(m_max_star_count);
    std::generate(m_ratings.begin(), m_ratings.end(),
        [n = 1] () mutable { return QString::number(n++); });
}

QSize StarWidget::sizeHint() const
{
    return PaintingScaleFactor * QSize(m_max_star_count, 1);
}

void StarWidget::set_ratings(const QStringList& ratings)
{
    m_ratings = ratings;
    m_max_star_count = std::ssize(m_ratings);
    m_star_count = m_max_star_count;
}

int StarWidget::get_rating() const
{
    return m_star_count;
}

void StarWidget::set_rating(int rating)
{
    m_star_count = rating;
}

void StarWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    paint(&painter, rect(), palette(), EditMode::Editable);
}

void StarWidget::mouseMoveEvent(QMouseEvent *event)
{
    const int star = star_at_position(event->position().toPoint().x());

    if (star != -1)
    {
        setCursor(Qt::PointingHandCursor);

        QPoint screen_pos = mapToGlobal(event->pos());
        screen_pos += QPoint(10, 10);
        QToolTip::showText(screen_pos, m_ratings[star - 1]);

        m_is_on_fly = true;
        m_on_fly_star_count = star;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void StarWidget::mouseReleaseEvent(QMouseEvent *event)
{
    const int star = star_at_position(event->position().toPoint().x());

    if (star != -1)
    {
        m_star_count = star;
        update();
    }

    QWidget::mouseReleaseEvent(event);
}

void StarWidget::leaveEvent(QEvent* event)
{
    m_is_on_fly = false;
    update();
    QWidget::leaveEvent(event);
}

int StarWidget::star_at_position(int x) const
{
    const int star = (x / (sizeHint().width()
                           / m_max_star_count)) + 1;
    if (star <= 0 || star > m_max_star_count)
        return -1;

    return star;
}

void StarWidget::paint(QPainter* painter, const QRect& rect, const QPalette&, EditMode) const
{
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);

    const int yOffset = (rect.height() - PaintingScaleFactor) / 2;
    painter->translate(rect.x(), rect.y() + yOffset);
    painter->scale(PaintingScaleFactor, PaintingScaleFactor);

    const QColor bgcolor = "#00A2E8";
    constexpr float pen_width = 0.04f;

    QPen pen0 = Qt::NoPen;

    QPen pen1;
    pen1.setJoinStyle(Qt::RoundJoin);
    pen1.setWidthF(pen_width);
    pen1.setColor(QPalette().color(QPalette::WindowText));

    QBrush brush(bgcolor);

    for (int i = 0; i < m_max_star_count; ++i)
    {
        if (!m_is_on_fly)
        {
            if (i < m_star_count)
                draw(painter, star_polygon, pen0, brush);
            else
                draw(painter, diamond_polygon, Qt::NoPen, brush);
        }
        else
        {
            if (i < m_star_count)
                draw(painter, star_polygon, i < m_on_fly_star_count ? pen1 : pen0, brush);
            else if (i < m_on_fly_star_count)
            {
                draw(painter, star_polygon, pen1, Qt::NoBrush);
                draw(painter, diamond_polygon, Qt::NoPen, brush);
            }
            else
                draw(painter, diamond_polygon, Qt::NoPen, brush);
        }
        painter->translate(1.0, 0.0);
    }

    painter->restore();
}

void StarWidget::draw(QPainter* painter, const QPolygonF& polygon, const QPen& pen, const QBrush& brush) const
{
    painter->setPen(pen);
    painter->setBrush(brush);
    painter->drawPolygon(polygon, Qt::WindingFill);
}
