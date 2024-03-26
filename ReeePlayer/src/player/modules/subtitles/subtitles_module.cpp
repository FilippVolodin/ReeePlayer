#include <subtitles_module.h>
#include <subtitles_module_view.h>
#include <qsubtitles.h>
#include <player_types.h>
#include <app.h>
#include <clip_storage.h>
#include <subtitles_list.h>

#include <ui_player_window.h>

#include <QDockWidget>

SubtitlesModule::SubtitlesModule(int subs_id, App* app, SubtitlesList* subtitles_list, ModeMediator* mode_mediator, PlaybackMediator* playback_mediator) :
    m_subs_id(subs_id),
    m_app(app),
    m_subtitles_list(subtitles_list),
    m_mode_mediator(mode_mediator),
    m_playback_mediator(playback_mediator)
{
    connect(m_mode_mediator, &ModeMediator::mode_changed, this, &SubtitlesModule::set_mode);

    connect(m_playback_mediator, &PlaybackMediator::time_changed, this, &SubtitlesModule::set_time);
    connect(m_playback_mediator, &PlaybackMediator::file_changed, this, &SubtitlesModule::set_file);
}

void SubtitlesModule::setup_player(Ui_PlayerWindow* player_window)
{
    if (m_subs_id == 0)
        m_view = player_window->edtSubtitles1;
    else if (m_subs_id == 1)
        m_view = player_window->edtSubtitles2;

    //QDockWidget* dock_widget = new QDockWidget(player_window);
    //// dock_widget->setObjectName("dock_widget");
    //dock_widget->setFeatures(QDockWidget::DockWidgetFeatureMask);
    //QWidget* dock_widget_contents = new QWidget();
    //// dock_widget_contents->setObjectName("dock_widget_contents");
    //QVBoxLayout* layout = new QVBoxLayout(dock_widget_contents);
    //layout->setSpacing(0);
    //layout->setContentsMargins(0, 0, 0, 0);
    //m_view = new pm::SubtitlesView(dock_widget_contents);
    //// edtSubtitles1->setObjectName("edtSubtitles1");
    //QSizePolicy size_policy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    //size_policy.setHorizontalStretch(0);
    //size_policy.setVerticalStretch(1);
    //size_policy.setHeightForWidth(m_view->sizePolicy().hasHeightForWidth());
    //m_view->setSizePolicy(size_policy);
    //m_view->setMinimumSize(QSize(0, 70));
    //m_view->setMaximumSize(QSize(16777215, 16777215));

    //layout->addWidget(m_view);

    //dock_widget->setWidget(dock_widget_contents);
    //player_window->addDockWidget(Qt::BottomDockWidgetArea, dock_widget);

    connect(m_view, &SubtitlesView::on_show_always, this, &SubtitlesModule::on_show_always_changed);
    connect(m_view, &SubtitlesView::on_insert_clicked, this, &SubtitlesModule::on_insert_clicked);
    connect(m_view, &SubtitlesView::on_file_changed, this, &SubtitlesModule::on_subs_file_changed);
}

void SubtitlesModule::load(const Clip& clip)
{
    const ClipUserData* data = clip.get_user_data();
    if (m_subs_id < std::ssize(data->subtitles))
        m_view->set_text(data->subtitles[m_subs_id]);
}

void SubtitlesModule::save(ClipUserData& data)
{
    if (m_subs_id < std::ssize(data.subtitles))
        data.subtitles[m_subs_id] = m_view->get_text();
}

void SubtitlesModule::set_file(const File*)
{
    //QFileInfo fileinfo(filename);
    //SubsCollection subs = m_app->get_subtitles(filename);
    //QString complete_base_file_name = fileinfo.completeBaseName();
    //std::vector<QString> short_sub_names;
    //for (int si = 0; si < subs.files.size(); ++si)
    //{
    //    const QString& s = subs.files[si];
    //    QString subs_filename = fileinfo.absolutePath() + "/" + s;
    //    m_subtitle_files.push_back(subs_filename);

    //    QFileInfo sub_info(s);
    //    QString subs_file_name = sub_info.fileName();
    //    QString short_name;
    //    if (subs_file_name.startsWith(complete_base_file_name))
    //    {
    //        QString suffix = subs_file_name.mid(complete_base_file_name.length());
    //        short_sub_names.push_back(suffix);
    //    }
    //    else
    //    {
    //        short_sub_names.push_back(s);
    //    }
    //}

    const std::vector<QString>& short_names = m_subtitles_list->get_short_names();
    const std::vector<QString>& files = m_subtitles_list->get_files();

    QComboBox* cmb = m_view->get_combobox();
    cmb->blockSignals(true);
    for (int si = 0; si < std::ssize(short_names); ++si)
    {
        cmb->addItem(short_names[si]);
    }
    cmb->addItem("<none>");

    int idx = m_subtitles_list->get_preferred_file_index(m_subs_id);
    if (idx == -1)
        idx = cmb->count() - 1;
    else
        m_subtitles = m_subtitles_list->fetch(m_subs_id, files[idx]);

    cmb->setCurrentIndex(idx);

    cmb->blockSignals(false);
}

void SubtitlesModule::set_mode(PlayerWindowMode mode)
{
    if (mode == PlayerWindowMode::Watching)
    {
        m_view->set_show_subs_files(true);
        m_view->set_show_offset_buttons(true);
        m_view->set_editable(false);
        m_view->set_show_insert_buttons(false);
    }
    else if (mode == PlayerWindowMode::AddingClip)
    {
        m_view->set_show_subs_files(false);
        m_view->set_show_offset_buttons(false);
        m_view->set_editable(true);
        m_view->set_show_insert_buttons(true);

        m_view->reset_insert_counters();
        update_insert_button(-1);
        update_insert_button(1);
    }
}

void SubtitlesModule::set_time(int time)
{
    update_cue(time);
}

void SubtitlesModule::set_subtitles(const QString& filename)
{
    m_subtitles = m_subtitles_list->fetch(m_subs_id, filename);
    update_cue(m_playback_mediator->get_time());
}

void SubtitlesModule::update_cue(int time)
{
    const qsubs::ICue* cue = nullptr;
    if (m_subtitles)
    {
        int offset = m_view->get_offset();
        cue = m_subtitles->pick_cue(time + offset, false);
    }

    if (cue)
    {
        if (m_current_cue != cue)
        {
            m_view->set_text(cue->get_text());
            m_view->next();

            //bool first_non_empty_subs = true;
            //for (int i = 0; i < index; ++i)
            //    if (m_subtitles)
            //        first_non_empty_subs = false;

            //if (first_non_empty_subs)
            //{
            //    // TODO check
            //    //m_edt_loop_a->setValue(round50(cue->get_start_time()));
            //    //m_edt_loop_b->setValue(round50(cue->get_end_time()));
            //}
        }
    }
    else
    {
        m_view->clear();
    }
    m_current_cue = cue;
}

void SubtitlesModule::update_insert_button(int value)
{
    QString text;
    if (m_subtitles && m_current_cue)
    {
        int idx = m_current_cue->get_index() + value;
        const qsubs::ICue* cue = m_subtitles->get_cue(idx);
        if (cue)
        {
            text = cue->get_text();
        }
    }
    if (value < 0)
        m_view->set_insert_left_button_tip(text);
    else if (value > 0)
        m_view->set_insert_right_button_tip(text);
}

void SubtitlesModule::on_show_always_changed(bool show)
{
    QString mode_str;
    switch (m_mode_mediator->get_mode())
    {
    case PlayerWindowMode::Watching: mode_str = "watching"; break;
    case PlayerWindowMode::Repeating: mode_str = "repeating"; break;
    case PlayerWindowMode::WatchingClip: mode_str = "watching_clip"; break;
    default: return;
    }

    QString setting_name =
        QString("show_subtitles_%1_%2").arg(m_subs_id).arg(mode_str);

    m_app->set_setting("gui", setting_name, show ? "1" : "0");
}

void SubtitlesModule::on_insert_clicked(int value)
{
    // const qsubs::ISubtitles* subs = m_subtitles[index].get();
    // m_subtitles_list->

    if (m_current_cue)
    {
        int idx = m_current_cue->get_index() + value;
        const qsubs::ICue* n_cue = m_subtitles->get_cue(idx);
        if (n_cue)
        {
            QString text = m_view->get_text();
            if (value < 0)
            {
                text = n_cue->get_text() + "\n" + text;
            }
            else
            {
                text += "\n" + n_cue->get_text();
            }
            m_view->set_text(text);
            update_insert_button(value + (value < 0 ? -1 : 1));
        }
    }
}

void SubtitlesModule::on_subs_file_changed(int file_index)
{
    QString filename = m_subtitles_list->get_files()[file_index];
    set_subtitles(filename);
    save_subs_priority();
}

void SubtitlesModule::save_subs_priority()
{
    // TODO
    //SubsCollection subs;
    //subs.indices.resize(NUM_SUBS_VIEWS);
    //subs.files = m_subtitle_files;

    //for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    //{
    //    int idx = m_subtitle_views[i]->get_combobox()->currentIndex();
    //    if (idx >= subs.files.size())
    //        idx = -1;
    //    subs.indices[i] = idx;
    //}

    //QString filename = m_clip_queue->get_file_path();
    //m_app->save_subtitle_priority(filename, subs);
}