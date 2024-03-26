#include <subtitles_list.h>

SubtitlesList::SubtitlesList(App* app, const QString& media_file)
{
    QFileInfo fileinfo(media_file);
    m_subs = app->get_subtitles(media_file);
    QString complete_base_file_name = fileinfo.completeBaseName();
    m_short_sub_names.reserve(std::ssize(m_subs.files));
    for (int si = 0; si < std::ssize(m_subs.files); ++si)
    {
        const QString& s = m_subs.files[si];
        QString subs_filename = fileinfo.absolutePath() + "/" + s;
        m_subtitle_files.push_back(subs_filename);

        QFileInfo sub_info(s);
        QString subs_file_name = sub_info.fileName();
        QString short_name;
        if (subs_file_name.startsWith(complete_base_file_name))
        {
            QString suffix = subs_file_name.mid(complete_base_file_name.length());
            m_short_sub_names.push_back(suffix);
        }
        else
        {
            m_short_sub_names.push_back(s);
        }
    }
}

const qsubs::ISubtitles* SubtitlesList::fetch(int id, const QString& subs_file)
{
    QFileInfo fi(subs_file);
    if (!fi.exists())
        return nullptr;

    const QString& file_name = fi.canonicalFilePath();
    auto it = m_subtitles.find(file_name);
    if (it != std::end(m_subtitles))
    {
        id_assign(id, it->second.get());
        return it->second.get();
    }
    else
    {
        qsubs::ISubtitlesPtr subtitles = qsubs::load(subs_file);
        m_subtitles[file_name] = subtitles;
        id_assign(id, subtitles.get());
        return subtitles.get();
    }
}

const qsubs::ISubtitles* SubtitlesList::get_primary() const
{
    auto it = std::find_if(std::begin(m_subtitles_index), std::end(m_subtitles_index),
        [](const qsubs::ISubtitles* s) { return s != nullptr; });
    return it != std::end(m_subtitles_index) ? *it : nullptr;
}

const std::vector<QString>& SubtitlesList::get_files() const
{
    return m_subtitle_files;
}

const std::vector<QString>& SubtitlesList::get_short_names() const
{
    return m_short_sub_names;
}

int SubtitlesList::get_preferred_file_index(int index) const
{
    if (index < 0 || index >= std::ssize(m_subs.indices))
        return -1;

    return m_subs.indices[index];
}

std::optional<PlaybackTime> SubtitlesList::rewind(PlaybackTime time, PlaybackTimeDiff diff) const
{
    // TODO 
    return time + diff;
}

void SubtitlesList::id_assign(int index, const qsubs::ISubtitles* subs)
{
    if (index < 0)
        return;

    if (index >= std::ssize(m_subtitles_index))
        m_subtitles_index.resize(index + 1);

    m_subtitles_index[index] = subs;
}
