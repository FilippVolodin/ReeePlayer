#include "pch.h"
#include "clip_storage.h"
#include "library.h"

void Clip::set_file(File* file)
{
    m_file = file;
}

Clip::Clip()
{
    using namespace std::chrono;
    time_t cur_time = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    set_adding_time(cur_time);
}

const File* Clip::get_file() const
{
    return m_file;
}

File* Clip::get_file()
{
    return m_file;
}

std::time_t Clip::get_begin() const
{
    return m_begin;
}

void Clip::set_begin(std::time_t begin)
{
    if (begin != m_begin)
    {
        m_begin = begin;
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

std::time_t Clip::get_end() const
{
    return m_end;
}

void Clip::set_end(std::time_t end)
{
    if (end != m_end)
    {
        m_end = end;
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

std::time_t Clip::get_adding_time() const
{
    return m_added;
}

void Clip::set_adding_time(std::time_t time)
{
    if (time != m_added)
    {
        m_added = time;
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

std::time_t Clip::get_rep_time() const
{
    return m_rep_time;
}

void Clip::set_rep_time(std::time_t rep_time)
{
    if (rep_time != m_rep_time)
    {
        m_rep_time = rep_time;
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

void Clip::add_repeat(std::time_t time)
{
    m_repeats.push_back(time);
    m_rep_time = time;
    if (m_file)
        m_file->get_library()->clip_changed(this);
}

const std::vector<std::time_t>& Clip::get_repeats() const
{
    return m_repeats;
}

void Clip::set_repeats(std::vector<std::time_t> repeats)
{
    m_repeats = std::move(repeats);
}

float Clip::get_level() const
{
    return m_level;
}

void Clip::set_level(float level)
{
    if (level != m_level)
    {
        m_level = level;
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

QString Clip::get_subtitle(int index) const
{
    return m_subtitles[index];
}

const std::vector<QString>& Clip::get_subtitles() const
{
    return m_subtitles;
}

void Clip::set_subtitles(std::vector<QString>&& subtitles)
{
    if (subtitles != m_subtitles)
    {
        m_subtitles = std::move(subtitles);
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

File::File(Library* library, const QString& path)
    : m_library(library), m_path(path)
{
}

File::~File()
{
    qDeleteAll(m_clips);
}

Library* File::get_library()
{
    return m_library;
}

QString File::get_path() const
{
    return m_path;
}

int File::get_num_clips() const
{
    return m_clips.size();
}

const Clips& File::get_clips() const
{
    return m_clips;
}

void File::add_clip(Clip* clip)
{
    clip->set_file(this);
    m_clips.insert(clip);
    m_library->clip_added(clip);
}

void File::remove_clip(Clip* clip)
{
    for (auto it = m_clips.begin(); it != m_clips.end(); ++it)
    {
        if (*it == clip)
        {
            m_clips.erase(it);
            break;
        }
    }

    m_library->clip_removed(clip);
    delete clip;
}

int File::get_player_time() const
{
    return m_player_time;
}

void File::set_player_time(int player_time)
{
    if (player_time != m_player_time)
    {
        m_player_time = player_time;
        get_library()->file_changed(this);
    }
}

int File::get_length() const
{
    return m_length;
}

void File::set_length(int length)
{
    if (length != m_length)
    {
        m_length = length;
        get_library()->file_changed(this);
    }
}
