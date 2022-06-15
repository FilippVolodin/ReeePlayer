#include "pch.h"
#include "session.h"
#include "repetition_model.h"
#include "clip_storage.h"
#include "app.h"
#include "library.h"

ClipPriorityCmp::ClipPriorityCmp(time_t cur_time)
    : m_cur_time(cur_time)
{
}

bool ClipPriorityCmp::operator()(const Clip* lhs, const Clip* rhs)
{
    float p1 = get_priority(m_cur_time, lhs->get_rep_time(), lhs->get_level());
    float p2 = get_priority(m_cur_time, rhs->get_rep_time(), rhs->get_level());
    return p1 > p2;
}

Session::Session(Library* library, const std::vector<File*>& files)
    : m_library(library)
{
    connect(m_library, &Library::clip_removed_sig, this, &Session::remove_clip);

    m_showed_clips.clear();
    m_showing_clip_index = -1;

    for (const File* file : files)
    {
        const Clips& clips = file->get_clips();
        m_clips.insert(m_clips.end(), clips.begin(), clips.end());
    }
}

Session::Session(Library*, const std::vector<Clip*>& clips) : m_clips(clips)
{
}

Session::~Session()
{
}

bool Session::has_prev_clip() const
{
    return m_showing_clip_index > 0;
}

bool Session::has_next_clip() const
{
    return m_showing_clip_index + 1 < m_showed_clips.size();
}

Clip* Session::get_prev_clip()
{
    if (!has_prev_clip())
        return nullptr;
        
    --m_showing_clip_index;
    return m_showed_clips[m_showing_clip_index];
}

Clip* Session::get_next_clip()
{
    if (m_clips.empty())
        return nullptr;

    Clip* clip;
    if (m_showing_clip_index + 1 < m_showed_clips.size())
    {
        clip = m_showed_clips[m_showing_clip_index + 1];
    }
    else
    {
        auto it = std::min_element(m_clips.begin(), m_clips.end(), ClipPriorityCmp(now()));
        if (it == m_clips.end())
            return nullptr;

        clip = *it;
        m_showed_clips.push_back(clip);
    }

    ++m_showing_clip_index;
    return clip;
}

void Session::remove_clip(Clip* clip)
{
    int showed_num = std::count(
        m_showed_clips.begin(),
        m_showed_clips.begin() + m_showing_clip_index + 1,
        clip
    );
    m_showing_clip_index -= showed_num;

    m_showed_clips.erase(
        remove(m_showed_clips.begin(),
            m_showed_clips.end(), clip),
        m_showed_clips.end());

    auto it = std::find(m_clips.begin(), m_clips.end(), clip);
    if (it != m_clips.end())
        m_clips.erase(it);
}

int Session::remain_clips()
{
    int64_t cur = now();

    auto need_to_repeat = [cur](const Clip* clip) -> bool
    {
        int64_t ri = get_repetititon_interval(clip->get_level()).begin;
        return cur >= (clip->get_rep_time() + ri) || clip->get_level() < 0.001;
    };

    return std::count_if(m_clips.begin(), m_clips.end(), need_to_repeat);
}

int Session::get_num_clips() const
{
    return m_clips.size();
}
