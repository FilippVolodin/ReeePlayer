#include <clip_module.h>
#include <spinbox.h>

#include <ui_player_window.h>

ClipModule::ClipModule(App* app, ModeMediator* mode_mediator, PlaybackMediator* playback_mediator)
    : m_app(app), m_mode_mediator(mode_mediator), m_playback_mediator(playback_mediator)
{
    connect(m_mode_mediator, &ModeMediator::mode_changed, this, &ClipModule::set_mode);
}

void ClipModule::setup_player(Ui_PlayerWindow* pw)
{
    m_edt_loop_a = new SpinBox();
    m_edt_loop_a_action = pw->toolBar->addWidget(m_edt_loop_a);
    connect(m_edt_loop_a, &SpinBox::value_changed,
        this, &ClipModule::on_edt_loop_a_value_changed);

    m_edt_loop_b = new SpinBox();
    m_edt_loop_b_action = pw->toolBar->addWidget(m_edt_loop_b);
    connect(m_edt_loop_b, &SpinBox::value_changed,
        this, &ClipModule::on_edt_loop_b_value_changed);

    m_act_remove_clip = pw->actRemoveClip;
    m_act_add_to_favorite = pw->actAddToFavorite;
}

void ClipModule::load(const Clip& clip)
{
}

void ClipModule::save(ClipUserData& clip)
{
}

void ClipModule::set_mode(PlayerWindowMode mode)
{
    if (is_film_mode(mode))
    {
        m_edt_loop_a_action->setVisible(false);
        m_edt_loop_b_action->setVisible(false);
        m_act_remove_clip->setVisible(false);
        m_act_add_to_favorite->setVisible(false);
    }
    else if (is_clip_mode(mode))
    {
        m_edt_loop_a_action->setVisible(true);
        m_edt_loop_b_action->setVisible(true);
        m_act_add_to_favorite->setVisible(true);
        if (mode == PlayerWindowMode::Repeating || mode == PlayerWindowMode::WatchingClip)
        {
            m_act_remove_clip->setVisible(true);
        }
        else
        {
            m_act_remove_clip->setVisible(false);
        }
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
