#include <clip_module.h>
#include <clip_storage.h>
#include <session.h>
#include <subtitles_list.h>

#include <spinbox.h>
#include <star_widget.h>

#include <ui_player_window.h>

ClipModule::ClipModule(App* app, const SubtitlesList* subtitles_list, IClipQueue* clip_queue,
    ModeMediator* mode_mediator, PlaybackMediator* playback_mediator, ClipMediator* clip_mediator) :
    m_app(app),
    m_subtitles_list(subtitles_list),
    m_clip_queue(clip_queue),
    m_mode_mediator(mode_mediator),
    m_playback_mediator(playback_mediator),
    m_clip_mediator(clip_mediator)
{
    connect(m_mode_mediator, &ModeMediator::mode_changed, this, &ClipModule::set_mode);
}

void ClipModule::setup_player(Ui_PlayerWindow* pw)
{
    m_pw = pw;

    pw->toolBar->addAction(pw->actAddClip);
    connect(pw->actAddClip, &QAction::triggered,
        this, &ClipModule::on_add_new_clip);

    pw->toolBar->addAction(pw->actSaveClip);
    connect(pw->actSaveClip, &QAction::triggered,
        this, &ClipModule::on_save_new_clip);

    pw->toolBar->addAction(pw->actCancelClip);
    connect(pw->actCancelClip, &QAction::triggered,
        this, &ClipModule::on_cancel_new_clip);

    m_edt_loop_a = new SpinBox();
    m_edt_loop_a_action = pw->toolBar->addWidget(m_edt_loop_a);
    connect(m_edt_loop_a, &SpinBox::value_changed,
        this, &ClipModule::on_edt_loop_a_value_changed);

    m_edt_loop_b = new SpinBox();
    m_edt_loop_b_action = pw->toolBar->addWidget(m_edt_loop_b);
    connect(m_edt_loop_b, &SpinBox::value_changed,
        this, &ClipModule::on_edt_loop_b_value_changed);

    m_star_widget = new StarWidget(4);
    m_star_widget->set_rating_names({ "Again", "Hard", "Medium", "Easy" });
    m_star_widget_action = pw->toolBar->addWidget(m_star_widget);
    connect(m_star_widget, &StarWidget::rating_changed, this, &ClipModule::on_rating_changed);

    m_act_prev_clip = pw->actPrevClip;
    pw->toolBar->addAction(m_act_prev_clip);
    connect(m_act_prev_clip, &QAction::triggered,
        this, &ClipModule::on_prev_clip);

    m_act_next_clip = pw->actNextClip;
    m_act_next_clip->setShortcuts({ tr("Return"), tr("Ctrl+Return") });
    pw->toolBar->addAction(m_act_next_clip);
    connect(m_act_next_clip, &QAction::triggered,
        this, &ClipModule::on_next_clip);

    m_act_add_to_favorite = pw->actAddToFavorite;
    m_pw->toolBar->addAction(m_pw->actAddToFavorite);

    m_act_remove_clip = pw->actRemoveClip;
    m_pw->toolBar->addAction(m_pw->actRemoveClip);

    m_clip_reviewed_shortcut = new QShortcut(pw->centralwidget);
    m_clip_reviewed_shortcut->setKeys({ tr("Return"), tr("Ctrl+Return") });
    connect(m_clip_reviewed_shortcut, &QShortcut::activated,
        this, &ClipModule::clip_reviewed);

    m_act_replay = pw->actReplayClip;
    m_btn_replay = pw->btnReplayClip;
    m_btn_replay->setDefaultAction(m_act_replay);
    connect(m_act_replay, &QAction::triggered,
        this, &ClipModule::on_replay);
}

void ClipModule::load(const Clip& clip)
{
    const ClipUserData* data = clip.get_user_data();
    m_edt_loop_a->setValue(data->begin);
    m_edt_loop_b->setValue(data->end);
    m_pw->actAddToFavorite->setChecked(data->is_favorite);

    if (m_mode_mediator->get_mode() == PlayerWindowMode::Repeating)
    {
        const srs::ICard* card = clip.get_card();
        bool use_rating = false;
        m_num_replays = 1;
        if (card)
        {
            const srs::IModel* model = card->get_model();
            use_rating = model->use_rating();
            int rating = card->get_model()->get_default_rating(m_num_replays);
            m_star_widget->set_rating(rating + 1);

            std::vector<Duration> due_intervals = card->get_due_intervals(now());
            QString s = get_interval_str(due_intervals[rating]);
            // TODO
            // m_lbl_info->setText("Due: " + s);

            QStringList dues_list;
            std::ranges::transform(due_intervals, std::back_inserter(dues_list),
                [](Duration d) {return get_interval_str(d); });
            m_star_widget->set_rating_comments(dues_list);
        }

        bool is_reviewing = m_clip_queue->is_reviewing();
        m_clip_reviewed_shortcut->setEnabled(is_reviewing);

        m_star_widget_action->setVisible(is_reviewing && use_rating);
    }

    m_act_next_clip->setEnabled(m_clip_queue->has_next());
    m_act_prev_clip->setEnabled(m_clip_queue->has_prev());

    m_playback_mediator->play(data->begin, data->end);
}

void ClipModule::save(ClipUserData& clip)
{
    clip.begin = m_edt_loop_a->value();
    clip.end = m_edt_loop_b->value();
    clip.is_favorite = m_pw->actAddToFavorite->isChecked();
}

void ClipModule::set_mode(PlayerWindowMode mode)
{
    if (is_film_mode(mode))
    {
        m_pw->actAddClip->setVisible(true);
        m_pw->actSaveClip->setVisible(false);
        m_pw->actCancelClip->setVisible(false);
        m_edt_loop_a_action->setVisible(false);
        m_edt_loop_b_action->setVisible(false);
        m_act_remove_clip->setVisible(false);
        m_act_add_to_favorite->setVisible(false);
        m_act_next_clip->setVisible(false);
        m_act_prev_clip->setVisible(false);
        m_star_widget_action->setVisible(false);
        m_clip_reviewed_shortcut->setEnabled(false);
        m_act_replay->setVisible(false);
        m_btn_replay->setVisible(false);
    }
    else if (is_clip_mode(mode))
    {
        m_edt_loop_a_action->setVisible(true);
        m_edt_loop_b_action->setVisible(true);
        m_act_add_to_favorite->setVisible(true);
        m_act_replay->setVisible(true);
        m_btn_replay->setVisible(true);
        if (mode == PlayerWindowMode::AddingClip)
        {
            m_pw->actAddClip->setVisible(false);
            m_pw->actSaveClip->setVisible(true);
            m_pw->actCancelClip->setVisible(true);
            m_act_remove_clip->setVisible(false);
            m_act_next_clip->setVisible(false);
            m_act_prev_clip->setVisible(false);
            m_star_widget_action->setVisible(false);
            m_clip_reviewed_shortcut->setEnabled(false);
        }
        else if (mode == PlayerWindowMode::Repeating)
        {
            m_pw->actAddClip->setVisible(false);
            m_pw->actSaveClip->setVisible(false);
            m_pw->actCancelClip->setVisible(false);
            m_act_remove_clip->setVisible(true);
            m_act_next_clip->setVisible(true);
            m_act_prev_clip->setVisible(true);
            m_star_widget_action->setVisible(true);
            m_clip_reviewed_shortcut->setEnabled(true);
        }
        else if (mode == PlayerWindowMode::WatchingClip)
        {
            m_pw->actAddClip->setVisible(false);
            m_pw->actSaveClip->setVisible(false);
            m_pw->actCancelClip->setVisible(false);
            m_act_remove_clip->setVisible(true);
            m_act_next_clip->setVisible(false);
            m_act_prev_clip->setVisible(false);
            m_star_widget_action->setVisible(false);
            m_clip_reviewed_shortcut->setEnabled(false);
        }
    }

    if (mode == PlayerWindowMode::AddingClip)
    {
        PlaybackTime time = m_playback_mediator->get_time();
        const qsubs::ISubtitles* primary_subs = m_subtitles_list->get_primary();
        const qsubs::ICue* cue = primary_subs ? primary_subs->pick_cue(time, false) : nullptr;
        PlaybackTime a = cue ? cue->get_start_time() : time - 1000;
        PlaybackTime b = cue ? cue->get_end_time() : time + 1000;
        m_edt_loop_a->setValue(a);
        m_edt_loop_b->setValue(b);
        m_playback_mediator->play(a, b);
        m_playback_mediator->set_default_rate(1.0f);
    }
}

void ClipModule::on_edt_loop_a_value_changed(int)
{
    if (m_mode_mediator->is_clip_mode())
    {
        int a = m_edt_loop_a->value();
        int b = m_edt_loop_b->value();
        m_playback_mediator->play(a, b);
    }
}

void ClipModule::on_edt_loop_b_value_changed(int)
{
    if (m_mode_mediator->is_clip_mode())
    {
        int a = m_edt_loop_a->value();
        int b = m_edt_loop_b->value();
        m_playback_mediator->play(std::max(a, b - 1000), b);
    }
}

void ClipModule::on_add_new_clip()
{
    m_mode_mediator->set_mode(PlayerWindowMode::AddingClip);
}

void ClipModule::on_save_new_clip()
{
    m_clip_queue->set_clip_user_data(m_clip_mediator->save());
    m_clip_queue->save_library();

    m_playback_mediator->set_trigger_time(-1);
    m_playback_mediator->set_time(m_edt_loop_a->value());
    m_playback_mediator->set_state(PlayState::Playing);

    m_mode_mediator->set_mode(PlayerWindowMode::Watching);
}

void ClipModule::on_cancel_new_clip()
{
    m_playback_mediator->set_trigger_time(-1);
    m_playback_mediator->set_state(PlayState::Playing);
    m_mode_mediator->set_mode(PlayerWindowMode::Watching);
}

void ClipModule::on_replay()
{
    int a = m_edt_loop_a->value();
    int b = m_edt_loop_b->value();
    m_playback_mediator->play(a, b);

    ++m_num_replays;
    const Clip* clip = m_clip_queue->get_clip();
    if (!clip)
        return;

    const srs::ICard* card = clip->get_card();
    if (!card)
        return;

    if (card->get_model()->use_rating())
    {
        int rating = card->get_model()->get_default_rating(m_num_replays);
        m_star_widget->set_rating(rating + 1);
    }
}

void ClipModule::on_next_clip()
{
    if (m_clip_queue->has_next())
    {
        // TODO move to ::save
        m_clip_queue->set_removed(m_pw->actRemoveClip->isChecked());
        m_clip_queue->set_clip_user_data(m_clip_mediator->save());
        m_clip_queue->save_library();
        m_clip_queue->next();
        m_clip_mediator->load(*m_clip_queue->get_clip());
    }
}

void ClipModule::on_prev_clip()
{
    if (m_clip_queue->has_prev())
    {
        // TODO move to ::save
        m_clip_queue->set_removed(m_pw->actRemoveClip->isChecked());
        m_clip_queue->set_clip_user_data(m_clip_mediator->save());
        m_clip_queue->save_library();
        m_clip_queue->prev();
        m_clip_mediator->load(*m_clip_queue->get_clip());
    }
}

void ClipModule::on_rating_changed(int)
{
    clip_reviewed();
}

void ClipModule::clip_reviewed()
{
    m_clip_queue->set_clip_user_data(m_clip_mediator->save());
    // TODO move to ::save
    m_clip_queue->set_removed(m_pw->actRemoveClip->isChecked());
    if (m_clip_queue->is_reviewing())
    {
        int rating = m_star_widget->get_rating();
        m_clip_queue->repeat(rating - 1);
    }
    m_clip_queue->save_library();
    if (m_clip_queue->next())
    {
        m_clip_mediator->load(*m_clip_queue->get_clip());
    }
    else
    {
        QMessageBox::information(nullptr, tr("Information"),
            tr("No clips to repeat"));
    }
}
