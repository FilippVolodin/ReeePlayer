#ifndef STATS_WINDOW_H
#define STATS_WINDOW_H

#include "ui_stats_window.h"

class Library;
class File;

class StatsWindow : public QDialog
{
    Q_OBJECT

public:
    StatsWindow(const std::vector<File*>&, QWidget *parent = Q_NULLPTR);
    ~StatsWindow();

private:



    Ui::StatsWindow ui;

    std::vector<int> m_num_added_clips;
    std::vector<int> m_num_repeated_clips;
    std::vector<int> m_total_num_clips;
};

#endif