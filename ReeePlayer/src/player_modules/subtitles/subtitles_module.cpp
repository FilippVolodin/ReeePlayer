#include <subtitles_module.h>
#include <subtitles_module_view.h>
#include <qsubtitles.h>
#include <player_types.h>
#include <app.h>

#include <QDockWidget>

SubtitlesModule::SubtitlesModule(App* app, const PlayerContext* player_context)
    : m_app(app), m_player_context(player_context)
{
}

void SubtitlesModule::setup(QMainWindow* player_window)
{
    QDockWidget* dock_widget = new QDockWidget(player_window);
    // dock_widget->setObjectName("dock_widget");
    dock_widget->setFeatures(QDockWidget::DockWidgetFeatureMask);
    QWidget* dock_widget_contents = new QWidget();
    // dock_widget_contents->setObjectName("dock_widget_contents");
    QVBoxLayout* layout = new QVBoxLayout(dock_widget_contents);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    m_view = new pm::SubtitlesView(dock_widget_contents);
    // edtSubtitles1->setObjectName("edtSubtitles1");
    QSizePolicy size_policy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    size_policy.setHorizontalStretch(0);
    size_policy.setVerticalStretch(1);
    size_policy.setHeightForWidth(m_view->sizePolicy().hasHeightForWidth());
    m_view->setSizePolicy(size_policy);
    m_view->setMinimumSize(QSize(0, 70));
    m_view->setMaximumSize(QSize(16777215, 16777215));

    layout->addWidget(m_view);

    dock_widget->setWidget(dock_widget_contents);
    player_window->addDockWidget(Qt::BottomDockWidgetArea, dock_widget);
}

void SubtitlesModule::show_video(const QString& filename)
{
    QFileInfo fileinfo(filename);
    SubsCollection subs = m_app->get_subtitles(filename);
    QString complete_base_file_name = fileinfo.completeBaseName();
    std::vector<QString> short_sub_names;
    for (int si = 0; si < subs.files.size(); ++si)
    {
        const QString& s = subs.files[si];
        QString subs_filename = fileinfo.absolutePath() + "/" + s;
        m_subtitle_files.push_back(subs_filename);

        QFileInfo sub_info(s);
        QString subs_file_name = sub_info.fileName();
        QString short_name;
        if (subs_file_name.startsWith(complete_base_file_name))
        {
            QString suffix = subs_file_name.mid(complete_base_file_name.length());
            short_sub_names.push_back(suffix);
        }
        else
        {
            short_sub_names.push_back(s);
        }
    }

    QComboBox* cmb = m_view->get_combobox();
    cmb->blockSignals(true);
    for (int si = 0; si < subs.files.size(); ++si)
    {
        cmb->addItem(short_sub_names[si]);
    }
    cmb->addItem("<none>");

    int idx = subs.indices[0];
    if (idx == -1)
        idx = cmb->count() - 1;
    else
        set_subtitles(0, m_subtitle_files[idx]);

    cmb->setCurrentIndex(idx);

    cmb->blockSignals(false);
}

void SubtitlesModule::time_changed(int time)
{
    update_cue(0);
}

void SubtitlesModule::set_subtitles(int index, const QString& filename)
{
    if (!filename.isEmpty())
        m_subtitles = qsubs::load(filename);
    else
        m_subtitles.reset();

    update_cue(index);
}

void SubtitlesModule::update_cue(int index)
{
    int time = m_player_context->time;
    const qsubs::ISubtitles* subs = m_subtitles.get();
    const qsubs::ICue* cue = nullptr;
    if (subs)
    {
        int offset = m_view->get_offset();
        cue = subs->pick_cue(time + offset, false);
    }

    if (cue)
    {
        if (m_current_cue != cue)
        {
            m_view->set_text(cue->get_text());
            m_view->next();

            bool first_non_empty_subs = true;
            for (int i = 0; i < index; ++i)
                if (m_subtitles)
                    first_non_empty_subs = false;

            if (first_non_empty_subs)
            {
                // TODO check
                //m_edt_loop_a->setValue(round50(cue->get_start_time()));
                //m_edt_loop_b->setValue(round50(cue->get_end_time()));
            }
        }
    }
    else
    {
        m_view->clear();
    }
    m_current_cue = cue;
}