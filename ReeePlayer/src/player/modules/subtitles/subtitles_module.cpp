#include <subtitles_module.h>
#include <subtitles_module_view.h>
#include <qsubtitles.h>
#include <player_types.h>
#include <app.h>
#include <clip_storage.h>

#include <ui_player_window.h>

#include <QDockWidget>

SubtitlesModule::SubtitlesModule(int subs_index, App* app, ModeMediator* mode_mediator, PlaybackMediator* playback_mediator)
    : m_subs_index(subs_index), m_app(app), m_mode_mediator(mode_mediator), m_playback_mediator(playback_mediator)
{
    connect(m_mode_mediator, &ModeMediator::mode_changed, this, &SubtitlesModule::set_mode);

    connect(m_playback_mediator, &PlaybackMediator::time_changed, this, &SubtitlesModule::set_time);
    connect(m_playback_mediator, &PlaybackMediator::file_changed, this, &SubtitlesModule::set_file);
}

void SubtitlesModule::setup_player(Ui_PlayerWindow* player_window)
{
    if (m_subs_index == 0)
        m_view = player_window->edtSubtitles1;
    else if (m_subs_index == 1)
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
}

void SubtitlesModule::load(const Clip& clip)
{
    const ClipUserData* data = clip.get_user_data();
    if (m_subs_index < std::ssize(data->subtitles))
        m_view->set_text(data->subtitles[m_subs_index]);
}

void SubtitlesModule::save(ClipUserData& data)
{
    if (m_subs_index < std::ssize(data.subtitles))
        data.subtitles[m_subs_index] = m_view->get_text();
}

void SubtitlesModule::set_file(const QString& filename)
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
        set_subtitles(m_subs_index, m_subtitle_files[idx]);

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

void SubtitlesModule::set_subtitles(int index, const QString& filename)
{
    if (!filename.isEmpty())
        m_subtitles = qsubs::load(filename);
    else
        m_subtitles.reset();

    update_cue(0);
}

void SubtitlesModule::update_cue(int time)
{
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
    const qsubs::ISubtitles* subs = m_subtitles.get();
    QString text;
    if (subs && m_current_cue)
    {
        int idx = m_current_cue->get_index() + value;
        const qsubs::ICue* cue = subs->get_cue(idx);
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
