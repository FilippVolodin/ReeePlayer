#include "pch.h"
#include "app.h"

#include "library.h"

App::App()
{
    const char * const vlc_args[] =
    {
        "--play-and-pause",
        "--intf=dummy",
        "--no-spu",
    };
    m_vlc_inst = libvlc_new(3, vlc_args);

    QStringList paths = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    if (!paths.isEmpty())
        m_settings = std::make_unique<QSettings>(paths.first() + "/settings.ini", QSettings::Format::IniFormat);
    else
        m_settings = std::make_unique<QSettings>();

    m_card_factory = std::make_unique<srs::Factory>();
}

App::~App()
{
    if (m_library)
    {
        std::map<QString, bool> expanded_map;
        get_expanded(m_library->get_root(), expanded_map);
        for (auto it = expanded_map.begin(); it != expanded_map.end(); ++it)
        {
            QString path = it->first;
            bool expanded = it->second;
            QSettings* s = get_settings();
            QString key = "expanded/" + path;
            if (s->contains(key))
            {
                if (!expanded)
                    s->remove(key);
            }
            else
            {
                if (expanded)
                    s->setValue(key, QString());
            }
        }
    }

    libvlc_release(m_vlc_inst);
}

void App::open_dir(const QString& filename)
{
    if (QFileInfo(filename).exists())
    {
        m_library = std::make_unique<Library>(get_settings(), filename);
        m_library->load(m_card_factory.get());
        m_settings->setValue("session/recent", filename);
    }
}

QString App::get_recent_dir() const
{
    return m_settings->value("session/recent").toString();
}

Library* App::get_library()
{
    return m_library.get();
}

QSettings* App::get_settings()
{
    return m_settings.get();
}

QVariant App::get_setting(const QString& group, const QString& key, const QVariant& default_value) const
{
    return m_settings->value(group + "/" + key, default_value);
}

void App::set_setting(const QString& group, const QString& key, const QVariant& value)
{
    m_settings->setValue(group + "/" + key, value);
}

libvlc_instance_t* App::get_vlc_instance() const
{
    return m_vlc_inst;
}

SubsCollection App::get_subtitles(const QString& video_file)
{
    return get_subtitles(video_file, m_settings->value("main/subs_priority").toString());
}

void App::save_subtitle_priority(const QString& video_file, const SubsCollection& subs)
{
    QString cur_priority = m_settings->value("main/subs_priority").toString();

    struct SubsPriorityItem
    {
        QString suffix;
        int index;
        bool operator==(SubsPriorityItem& r) { return suffix == r.suffix && index == r.index; }
    };
    std::list<SubsPriorityItem> subs_priorities;

    for (const QString& p : cur_priority.split('/', Qt::SkipEmptyParts))
    {
        QString suffix = p.left(p.size() - 1);
        bool ok;
        int index = p.right(1).toInt(&ok);
        if (ok)
        {
            subs_priorities.push_back({ suffix, index });
        }
    }

    std::map<QString, int> subs_suffixes;
    std::vector<QString> suffixes(subs.files.size());

    QFileInfo info(video_file);
    QString complete_name = info.completeBaseName();
    int fidx = 0;
    for (const QString& file : subs.files)
    {
        QFileInfo sub_info(file);
        QString subs_file_name = sub_info.fileName();
        if (subs_file_name.startsWith(complete_name))
        {
            QString sub_suffix = subs_file_name.mid(complete_name.length());
            subs_suffixes[sub_suffix] = fidx;
            suffixes[fidx] = sub_suffix;

        }
        ++fidx;
    }

    for (int i = 1; i >= 0; --i)
    {
        auto it = subs_priorities.begin();
        for (; it != subs_priorities.end(); ++it)
        {
            if (it->suffix.isEmpty() || subs_suffixes.find(it->suffix) != subs_suffixes.end())
            {
                break;
            }
        }

        int user_index = subs.indices[i];
        SubsPriorityItem sp_item;
        sp_item.index = i;
        if (user_index != -1)
            sp_item.suffix = suffixes[user_index];

        auto inserted_it = subs_priorities.insert(it, sp_item);
        ++inserted_it;
        for (auto erase_it = inserted_it; erase_it != subs_priorities.end();)
        {
            if (*erase_it == sp_item)
            {
                erase_it = subs_priorities.erase(erase_it);
            }
            else
            {
                ++erase_it;
            }
        }
    }

    QStringList res;
    for (const SubsPriorityItem& sp_item : subs_priorities)
    {
        res.push_back(sp_item.suffix + QString::number(sp_item.index));
    }
    m_settings->setValue("main/subs_priority", res.join("/"));
}

const srs::IFactory* App::get_srs_factory() const
{
    return m_card_factory.get();
}

SubsCollection App::get_subtitles(const QString& video_file, const QString& priorities)
{
    SubsCollection result;
    // TODO magic number
    result.indices.assign(2, -2);

    QFileInfo info(video_file);
    QString complete_name = info.completeBaseName();

    QDir dir(info.absolutePath());
    dir.setNameFilters({ "*.srt", "*.vtt" });
    dir.setFilter(QDir::Files | QDir::NoSymLinks);

    std::map<QString, int> exist_subs_suffixes;

    int num_files = 0;
    for (const QFileInfo& sub_fi : dir.entryInfoList())
    {
        QString subs_file_name = sub_fi.fileName();
        if (subs_file_name.startsWith(complete_name))
        {
            QString sub_suffix = subs_file_name.mid(complete_name.length());
            exist_subs_suffixes[sub_suffix] = num_files++;
            result.files.push_back(subs_file_name);
        }
    }

    struct SubsPriorityItem
    {
        QString suffix;
        int index;
    };
    std::list<SubsPriorityItem> subs_priorities;

    for (const QString& p : priorities.split('/', Qt::SkipEmptyParts))
    {
        QString suffix = p.left(p.size() - 1);
        bool ok;
        int index = p.right(1).toInt(&ok);
        if (ok)
        {
            subs_priorities.push_back({ suffix, index });
        }
    }

    // Iterate preferred priorities and check if subtitles file with given suffix exists
    std::set<int> suffix_index_set;

    for (const SubsPriorityItem& sp_item : subs_priorities)
    {
        int suffix_index = -2;
        auto it = exist_subs_suffixes.find(sp_item.suffix);
        if (it != exist_subs_suffixes.end())
            suffix_index = it->second;
        else if(sp_item.suffix.isEmpty())
            suffix_index = -1;
        else
            continue;

        if (sp_item.index < 0 || sp_item.index >= 2)
            continue;

        if (result.indices[sp_item.index] == -2)
        {
            if (suffix_index_set.find(suffix_index) == suffix_index_set.end())
            {
                result.indices[sp_item.index] = suffix_index;
                if(suffix_index != -1)
                    suffix_index_set.insert(suffix_index);
            }
        }
    }

    for (int& idx : result.indices)
    {
        if (idx != -2)
            continue;

        for (int i = 0; i < num_files; ++i)
        {
            if (suffix_index_set.find(i) == suffix_index_set.end())
            {
                idx = i;
                suffix_index_set.insert(idx);
                break;
            }
        }

        if (idx == -2)
            idx = -1;
    }

    return result;
}
