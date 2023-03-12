#include "pch.h"
#include "session.h"
#include "repetition_model.h"
#include "clip_storage.h"
#include "app.h"
#include "library.h"
#include <srs_icard.h>

ClipPriorityCmp::ClipPriorityCmp(TimePoint cur_time)
    : m_cur_time(cur_time)
{
}

bool ClipPriorityCmp::operator()(const Clip* lhs, const Clip* rhs)
{
    float p1 = lhs->get_card() ?
        lhs->get_card()->get_priority(m_cur_time) : 0.0f;
    float p2 = rhs->get_card() ?
        rhs->get_card()->get_priority(m_cur_time) : 0.0f;
    return p1 > p2;
}

struct Overdue
{
    Overdue(TimePoint cur_time) noexcept
        : cur_time(cur_time)
    {
    }

    bool operator()(const Clip* clip) const
    {
        if (!clip->get_card())
            return false;

        return clip->get_card()->is_due(cur_time) && !clip->is_removed();
    }
    TimePoint cur_time;
};

TodayClipStat::TodayClipStat(const Library* library, const File* file)
{
    QDate today = QDateTime::currentDateTime().date();

    auto today_repeated_count = [&today](int count, const File* file)
    {
        for (const Clip* clip : file->get_clips())
        {
            for (TimePoint time : clip->get_repeats())
            {
                auto time_sec = time.time_since_epoch().count();
                QDate date = QDateTime::fromSecsSinceEpoch(time_sec).date();
                if (date == today)
                    ++count;
            }
        }
        return count;
    };

    auto today_added_count = [&today](int count, const File* file)
    {
        for (const Clip* clip : file->get_clips())
        {
            if (clip->get_adding_time() != TimePoint(Duration::zero()))
            {
                auto time_sec = clip->get_adding_time().time_since_epoch().count();
                QDate date = QDateTime::fromSecsSinceEpoch(time_sec).date();
                if (date == today)
                    ++count;
            }
        }
        return count;
    };

    LibraryItem* root = library->get_root();
    std::vector<File*> files = get_files({ root });

    m_repeated_count = std::accumulate(files.begin(), files.end(), 0, today_repeated_count);
    m_added_count = std::accumulate(files.begin(), files.end(), 0, today_added_count);
    if (file)
        m_added_count_for_file = today_added_count(0, file);
}

void TodayClipStat::inc_added()
{
    ++m_added_count;
    ++m_added_count_for_file;
}

void TodayClipStat::inc_repeated()
{
    ++m_repeated_count;
}

int TodayClipStat::get_added_count() const
{
    return m_added_count;
}

int TodayClipStat::get_added_count_for_file() const
{
    return m_added_count_for_file;
}

int TodayClipStat::get_repeated_count() const
{
    return m_repeated_count;
}

IClipQueue::~IClipQueue()
{
}

BaseClipQueue::BaseClipQueue(Library* library) :
    m_library(library),
    m_today_clip_stat(std::make_unique<TodayClipStat>(library, nullptr))
{
}

BaseClipQueue::BaseClipQueue(Library* library, const File* file) :
    m_library(library),
    m_today_clip_stat(std::make_unique<TodayClipStat>(library, file))
{
}

const Clip* BaseClipQueue::get_clip() const
{
    return m_current_clip;
}

const ClipUserData* BaseClipQueue::get_clip_user_data() const
{
    if (!m_current_clip)
        return nullptr;
    return m_current_clip->get_user_data();
}

void BaseClipQueue::set_clip_user_data(std::unique_ptr<ClipUserData> data)
{
    Clip* clip = get_current_clip();
    if (clip)
        clip->set_user_data(std::move(data));
    m_library->save(clip);
}

void BaseClipQueue::set_removed(bool value)
{
    Clip* clip = get_current_clip();
    if (clip)
    {
        if (value)
            clip->set_removal_time(now());
        else
            clip->restore();
        m_library->save(clip);
    }
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
    if (file)
    {
        file->set_user_data(std::move(data));
        m_library->save(file);
    }
}

bool BaseClipQueue::is_reviewing() const
{
    return false;
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

int BaseClipQueue::overdue_count() const
{
    return 0;
}

void BaseClipQueue::repeat(int /*rating*/)
{
}

void BaseClipQueue::save_library()
{
    m_library->save();
}

const TodayClipStat* BaseClipQueue::get_today_clip_stat() const
{
    return m_today_clip_stat.get();
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
    if (clip)
        return clip->get_file();
    else
        return nullptr;
}

File* BaseClipQueue::get_current_file()
{
    Clip* clip = get_current_clip();
    if (clip)
        return clip->get_file();
    else
        return nullptr;
}

TodayClipStat* BaseClipQueue::get_today_clip_stat()
{
    return m_today_clip_stat.get();
}

RepeatingClipQueue::RepeatingClipQueue(Library* library, const std::vector<File*>& files)
    : BaseClipQueue(library)
{
    m_showed_clips.clear();
    m_showing_clip_index = -1;

    for (File* file : files)
    {
        auto clips = file->get_clips();
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

int RepeatingClipQueue::overdue_count() const
{
    TimePoint cur_time = now();

    auto need_to_repeat = [cur_time](const Clip* clip) -> bool
    {
        return clip->get_card() ? clip->get_card()->is_due(cur_time) : false;
    };

    return std::count_if(m_clips.begin(), m_clips.end(), need_to_repeat);
}

void RepeatingClipQueue::repeat(int rating)
{
    TimePoint cur = now();
    Clip* clip = get_current_clip();
    if (clip->get_card())
    {
        clip->get_card()->repeat(cur, rating);
        clip->add_repeat(cur);
        get_today_clip_stat()->inc_repeated();
    }
}

RepeatingClipQueueV2::RepeatingClipQueueV2(Library* library, const std::vector<File*>& files)
    : BaseClipQueue(library)
{
    Overdue overdue(now());
    for (File* file : files)
    {
        auto clips = file->get_clips();
        std::ranges::copy_if(clips, std::back_inserter(m_clips), overdue);
    }
    std::ranges::sort(m_clips, ClipPriorityCmp(now()));

    if (!m_clips.empty())
    {
        m_show_it = std::begin(m_clips);
        m_review_it = std::begin(m_clips);
        set_current_clip(*m_show_it);
    }
    else
    {
        m_show_it = std::end(m_clips);
        m_review_it = std::end(m_clips);
    }
}

RepeatingClipQueueV2::RepeatingClipQueueV2(Library* library, const std::vector<Clip*>& clips)
    : BaseClipQueue(library)
{
    std::ranges::copy_if(clips, std::back_inserter(m_clips), Overdue(now()));
    std::ranges::sort(m_clips, ClipPriorityCmp(now()));

    if (!m_clips.empty())
    {
        m_show_it = std::begin(m_clips);
        m_review_it = std::begin(m_clips);
        set_current_clip(*m_show_it);
    }
    else
    {
        m_show_it = std::end(m_clips);
        m_review_it = std::end(m_clips);
    }
}

RepeatingClipQueueV2::~RepeatingClipQueueV2()
{
}

bool RepeatingClipQueueV2::is_reviewing() const
{
    return m_show_it == m_review_it;
}

bool RepeatingClipQueueV2::has_next() const
{
    //if (m_showing_clip_index + 1 >= m_clips.size())
    //    return false;

    //return m_showing_clip_index < m_rewieved_clip_index;

    return find_next() != std::end(m_clips);
}

bool RepeatingClipQueueV2::has_prev() const
{
    //return m_showing_clip_index > 0;

    return find_prev() != std::end(m_clips);
}

bool RepeatingClipQueueV2::next()
{
    //if (!has_next())
    //    return false;

    //++m_showing_clip_index;
    //set_current_clip(m_clips[m_showing_clip_index]);
    //return true;

    It it = find_next();
    if (it == std::end(m_clips))
        return false;

    m_show_it = it;
    set_current_clip(*m_show_it);
    return true;
}

bool RepeatingClipQueueV2::prev()
{
    //if (!has_prev())
    //    return false;

    //--m_showing_clip_index;
    //set_current_clip(m_clips[m_showing_clip_index]);
    //return true;

    It it = find_prev();
    if (it == std::end(m_clips))
        return false;

    m_show_it = it;
    set_current_clip(*m_show_it);
    return true;
}

int RepeatingClipQueueV2::overdue_count() const
{
    return std::distance(m_review_it, std::end(m_clips));
}

void RepeatingClipQueueV2::repeat(int rating)
{
    if (!is_reviewing())
        return;

    TimePoint cur = now();
    Clip* clip = get_current_clip();
    if (clip->get_card())
    {
        clip->get_card()->repeat(cur, rating);
        clip->add_repeat(cur);
        get_today_clip_stat()->inc_repeated();
    }

    ++m_review_it;
}

RepeatingClipQueueV2::It RepeatingClipQueueV2::find_prev() const
{
    if (m_show_it != std::begin(m_clips))
        return std::prev(m_show_it);
    else 
        return std::end(m_clips);
}

RepeatingClipQueueV2::It RepeatingClipQueueV2::find_next() const
{
    if (m_show_it == m_review_it)
        return std::end(m_clips);
    
    return std::next(m_show_it);
}

AddingClipsQueue::AddingClipsQueue(Library* library, File* file, const srs::IFactory* factory)
    : BaseClipQueue(library, file), m_file(file), m_srs_factory(factory)
{
}

void AddingClipsQueue::set_clip_user_data(std::unique_ptr<ClipUserData> user_data)
{
    std::unique_ptr<Clip> new_clip = std::make_unique<Clip>();
    srs::ICardUPtr card = m_srs_factory->create_card();
    new_clip->set_card(std::move(card));
    new_clip->generate_uid();
    new_clip->set_adding_time(now());
    new_clip->set_user_data(std::move(user_data));

    Clip* clip = new_clip.get();
    m_file->add_clip(std::move(new_clip));
    set_current_clip(clip);

    get_today_clip_stat()->inc_added();
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
