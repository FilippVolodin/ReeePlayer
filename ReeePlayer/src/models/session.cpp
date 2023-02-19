#include "pch.h"
#include "session.h"
#include "repetition_model.h"
#include "clip_storage.h"
#include "app.h"
#include "library.h"
#include <srs_interfaces.h>

ClipPriorityCmp::ClipPriorityCmp(TimePoint cur_time)
    : m_cur_time(cur_time)
{
}

bool ClipPriorityCmp::operator()(const Clip* lhs, const Clip* rhs)
{
    float p1 = lhs->get_card() != nullptr ?
        lhs->get_card()->get_priority(m_cur_time) : 0.0f;
    float p2 = rhs->get_card() != nullptr ?
        rhs->get_card()->get_priority(m_cur_time) : 0.0f;
    return p1 > p2;
}

IClipQueue::~IClipQueue()
{
}

BaseClipQueue::BaseClipQueue(Library* library)
    : m_library(library)
{
}

const ClipUserData* BaseClipQueue::get_clip_user_data() const
{
    return get_current_clip()->get_user_data();
}

void BaseClipQueue::set_clip_user_data(std::unique_ptr<ClipUserData> data)
{
    Clip* clip = get_current_clip();
    if (clip != nullptr)
        clip->set_user_data(std::move(data));
    m_library->save(clip);
}

void BaseClipQueue::remove()
{
    Clip* clip = get_current_clip();
    File* file = clip->get_file();
    file->remove_clip(clip);
    clip = nullptr;
}

const QString BaseClipQueue::get_file_path() const
{
    return get_current_file()->get_path();
}

const FileUserData* BaseClipQueue::get_file_user_data() const
{
    return get_current_file()->get_user_data();
}

void BaseClipQueue::set_file_user_data(std::unique_ptr<FileUserData> data)
{
    File* file = get_current_file();
    if (file != nullptr)
    {
        file->set_user_data(std::move(data));
        m_library->save(file);
    }
}

bool BaseClipQueue::has_next() const
{
    return false;
}

bool BaseClipQueue::has_prev() const
{
    return false;
}

bool BaseClipQueue::next()
{
    return false;
}

bool BaseClipQueue::prev()
{
    return false;
}

int BaseClipQueue::remain_count() const
{
    return 0;
}

void BaseClipQueue::repeat(int rating)
{
}

void BaseClipQueue::save_library()
{
    m_library->save();
}

Clip* BaseClipQueue::get_current_clip()
{
    return m_current_clip;
}

const Clip* BaseClipQueue::get_current_clip() const
{
    return m_current_clip;
}

void BaseClipQueue::set_current_clip(Clip* clip)
{
    m_current_clip = clip;
}

const File* BaseClipQueue::get_current_file() const
{
    const Clip* clip = get_current_clip();
    if (clip != nullptr)
        return clip->get_file();
    else
        nullptr;
}

File* BaseClipQueue::get_current_file()
{
    Clip* clip = get_current_clip();
    if (clip != nullptr)
        return clip->get_file();
    else
        nullptr;
}

RepeatingClipQueue::RepeatingClipQueue(Library* library, const std::vector<File*>& files)
    : BaseClipQueue(library)
{
    m_showed_clips.clear();
    m_showing_clip_index = -1;

    for (const File* file : files)
    {
        const Clips& clips = file->get_clips();
        m_clips.insert(m_clips.end(), clips.begin(), clips.end());
    }
    next();
}

RepeatingClipQueue::RepeatingClipQueue(Library* library, const std::vector<Clip*>& clips)
    : BaseClipQueue(library), m_clips(clips)
{
    next();
}

RepeatingClipQueue::~RepeatingClipQueue()
{
}

void RepeatingClipQueue::remove()
{
    const Clip* clip = get_current_clip();
    int showed_num = std::count(
        m_showed_clips.begin(),
        m_showed_clips.begin() + m_showing_clip_index + 1,
        clip
    );
    m_showing_clip_index -= showed_num;

    m_showed_clips.erase(
        std::remove(m_showed_clips.begin(), m_showed_clips.end(), clip),
        m_showed_clips.end());

    auto it = std::find(m_clips.begin(), m_clips.end(), clip);
    if (it != m_clips.end())
        m_clips.erase(it);

    BaseClipQueue::remove();
}

bool RepeatingClipQueue::has_next() const
{
    return m_showing_clip_index + 1 < m_showed_clips.size();
}

bool RepeatingClipQueue::has_prev() const
{
    return m_showing_clip_index > 0;
}

bool RepeatingClipQueue::next()
{
    if (m_clips.empty())
        return false;

    Clip* clip;
    if (m_showing_clip_index + 1 < m_showed_clips.size())
    {
        clip = m_showed_clips[m_showing_clip_index + 1];
    }
    else
    {
        auto it = std::min_element(m_clips.begin(), m_clips.end(), ClipPriorityCmp(now()));
        if (it == m_clips.end())
            return false;

        clip = *it;
        m_showed_clips.push_back(clip);
    }

    ++m_showing_clip_index;
    set_current_clip(clip);
    return true;
}

bool RepeatingClipQueue::prev()
{
    if (!has_prev())
        return false;

    --m_showing_clip_index;
    set_current_clip(m_showed_clips[m_showing_clip_index]);
    return true;
}

int RepeatingClipQueue::remain_count() const
{
    TimePoint cur_time = now();

    auto need_to_repeat = [cur_time](const Clip* clip) -> bool
    {
        return clip->get_card() != nullptr ? clip->get_card()->is_due(cur_time) : false;
    };

    return std::count_if(m_clips.begin(), m_clips.end(), need_to_repeat);
}

void RepeatingClipQueue::repeat(int rating)
{
    TimePoint cur = now();
    Clip* clip = get_current_clip();
    if (clip->get_card() != nullptr)
    {
        clip->get_card()->repeat(cur, rating);
        clip->add_repeat(cur);
    }
}

AddingClipsQueue::AddingClipsQueue(Library* library, File* file, const srs::ICardFactory* factory)
    : BaseClipQueue(library), m_file(file), m_factory(factory)
{
}

void AddingClipsQueue::set_clip_user_data(std::unique_ptr<ClipUserData> user_data)
{
    Clip* new_clip = new Clip();
    srs::ICardUPtr card = m_factory->create();
    new_clip->set_card(std::move(card));
    new_clip->generate_uid();
    new_clip->set_adding_time(now());
    m_file->add_clip(new_clip);

    set_current_clip(new_clip);
    BaseClipQueue::set_clip_user_data(std::move(user_data));
}

const File* AddingClipsQueue::get_current_file() const
{
    return m_file;
}

File* AddingClipsQueue::get_current_file()
{
    return m_file;
}

WatchClipQueue::WatchClipQueue(Library* library, Clip* clip) : BaseClipQueue(library)
{
    set_current_clip(clip);
}
