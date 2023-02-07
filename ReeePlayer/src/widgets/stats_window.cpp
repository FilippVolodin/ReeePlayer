#include "stats_window.h"
#include "models/library.h"
#include "models/clip_storage.h"

enum class RadioButtons { Added = 0, Repeated, Total };

StatsWindow::StatsWindow(const std::vector<File*>& files, QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    bool seen_any_time = false;
    TimePoint first;
    TimePoint last;

    auto update_first_last = [&](TimePoint time)
    {
        if (!seen_any_time)
        {
            first = time;
            last = time;
            seen_any_time = true;
        }
        else
        {
            if (time < first)
                first = time;
            if (time > last)
                last = time;
        }
    };

    for (const File* file : files)
        for (const Clip* clip : file->get_clips())
        {
            update_first_last(clip->get_adding_time());
            for (TimePoint time : clip->get_repeats())
                update_first_last(time);
        }

    if (!seen_any_time)
        return;

    QDate first_day = QDateTime::fromSecsSinceEpoch(first.time_since_epoch().count()).date();
    QDate last_day = QDateTime::fromSecsSinceEpoch(last.time_since_epoch().count()).date();
    int num_days = first_day.daysTo(last_day) + 1;
    
    m_num_added_clips.resize(num_days, 0);
    m_num_repeated_clips.resize(num_days, 0);
    m_total_num_clips.resize(num_days, 0);

    auto get_rel_day = [first_day](TimePoint time)
    {
        QDate date = QDateTime::fromSecsSinceEpoch(time.time_since_epoch().count()).date();
        return first_day.daysTo(date);
    };

    for (const File* file : files)
        for (const Clip* clip : file->get_clips())
        {
            int rel_day = get_rel_day(clip->get_adding_time());
            if (rel_day >= 0 && rel_day < num_days)
            {
                ++m_num_added_clips[rel_day];
                ++m_total_num_clips[rel_day];
            }

            for (TimePoint time : clip->get_repeats())
            {
                rel_day = get_rel_day(time);
                if (rel_day >= 0 && rel_day < num_days)
                {
                    ++m_num_repeated_clips[rel_day];
                    ++m_total_num_clips[rel_day];
                }
            }
        }

    QDate today = QDateTime::currentDateTime().date();
    int rel_today = first_day.daysTo(today);
    if (rel_today >= 0 && rel_today < num_days)
    {
        ui.lblNumAddedClips->setText(QString::number(m_num_added_clips[rel_today]));
        ui.lblNumRepeatedClips->setText(QString::number(m_num_repeated_clips[rel_today]));
        ui.lblTotalNum->setText(QString::number(m_total_num_clips[rel_today]));
    }
    ui.heatmap->set_data(first_day, m_total_num_clips);

    QButtonGroup* buttonGroup = new QButtonGroup;
    buttonGroup->addButton(ui.rbAddedClips, (int)RadioButtons::Added);
    buttonGroup->addButton(ui.rbRepeatedClips, (int)RadioButtons::Repeated);
    buttonGroup->addButton(ui.rbTotal, (int)RadioButtons::Total);
    connect(buttonGroup, &QButtonGroup::idToggled,
        [&, first_day](int id, bool checked)
        {
            if (checked)
            {
                switch ((RadioButtons)id)
                {
                case RadioButtons::Added: ui.heatmap->set_data(first_day, m_num_added_clips); break;
                case RadioButtons::Repeated: ui.heatmap->set_data(first_day, m_num_repeated_clips); break;
                case RadioButtons::Total: ui.heatmap->set_data(first_day, m_total_num_clips); break;
                }
            }
        });
}

StatsWindow::~StatsWindow()
{
}
