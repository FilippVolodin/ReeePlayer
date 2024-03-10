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

void ClipMediator::save(ClipUserData& clip)
{
    for (ClipUnit* unit : m_units)
        unit->save(clip);
}
