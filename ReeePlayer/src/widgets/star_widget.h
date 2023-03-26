#ifndef STAR_WIDGET_H
#define STAR_WIDGET_H

#include <QWidget>

class StarWidget : public QWidget
{
    Q_OBJECT
public:
    enum class EditMode { Editable, ReadOnly };

    StarWidget(int star_count, QWidget* parent = nullptr);

    QSize sizeHint() const override;

    void set_rating_names(const QStringList&);
    void set_rating_comments(const QStringList&);

    int get_rating() const;
    void set_rating(int);

signals:
    void rating_changed(int);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent* event) override;
private:
    int star_at_position(int x) const;
    void paint(QPainter* painter, const QRect& rect,
        const QPalette& palette, EditMode mode) const;
    void draw(QPainter* painter, const QPolygonF&, const QPen&, const QBrush&) const;

    QPolygonF star_polygon;
    QPolygonF diamond_polygon;

    QStringList m_ratings;
    QStringList m_comments;
    int m_star_count = 5;
    int m_on_fly_star_count = 5;
    int m_max_star_count = 5;
    bool m_is_on_fly = false;
};

#endif
