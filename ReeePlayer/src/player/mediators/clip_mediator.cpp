#include <clip_mediator.h>
#include <clip_storage.h>

void ClipMediator::add_unit(ClipUnit* unit)
{
    m_units.push_back(unit);
}

void ClipMediator::load(const Clip& clip)
{
    for (ClipUnit* unit : m_units)
        unit->load(clip);
}

std::unique_ptr<ClipUserData> ClipMediator::save()
{
    std::unique_ptr<ClipUserData> user_data = std::make_unique<ClipUserData>();
    user_data->subtitles.resize(2);
    for (ClipUnit* unit : m_units)
        unit->save(*user_data.get());
    return user_data;
}

void ClipMediator::save(ClipUserData& clip)
{
    for (ClipUnit* unit : m_units)
        unit->save(clip);
}
