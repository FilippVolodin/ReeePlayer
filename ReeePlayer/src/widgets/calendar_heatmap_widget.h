#pragma once
#include <QDate>
#include <QtWidgets/QAbstractScrollArea>

struct Layout
{
    int scrollbar_width = 0;
    int scrollbar_height = 0;

    int internal_width = 0;
    int internal_height = 0;

    int left_pad = 0;

    int content_left = 0;
    int content_width = 0;

    int left_column_width = 0;

    int heatmap_left = 0;
    int heatmap_top = 0;
    int heatmap_width = 0;

    float weak_width = 0;
    float square_size = 0;

    int year_gap = 0;
    int year_interval_height = 0;
    int weak_height = 0;
};

class CalendarHeatmapWidget : public QAbstractScrollArea
{
    Q_OBJECT

public:
    CalendarHeatmapWidget(QWidget *parent = Q_NULLPTR);
    void set_data(QDate first_day, std::vector<int> data);
protected:
    void showEvent(QShowEvent* event);
    void resizeEvent(QResizeEvent* event);
    void paintEvent(QPaintEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
private:
    int get_count(int year, int day_index) const;

    QDate m_data_first_day;
    std::vector<int> m_data;

    Layout m_layout;

    bool m_is_data_valid = false;
    int m_min_year = 2022;
    int m_max_year = 2022;
    int m_max_value = 0;


    //QDate m_base_date;
};
