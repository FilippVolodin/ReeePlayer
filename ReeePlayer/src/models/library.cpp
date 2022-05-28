#include "pch.h"
#include "library.h"
#include "clip_storage.h"

static std::set<QString> audio_exts = { "3ga", "669", "a52", "aac", "ac3", "adt", "adts", "aif", "aifc", "aiff",
                         "amb", "amr", "aob", "ape", "au", "awb", "caf", "dts", "dsf", "dff", "flac", "it", "kar",
                         "m4a", "m4b", "m4p", "m5p", "mka", "mlp", "mod", "mpa", "mp1", "mp2", "mp3", "mpc", "mpga", "mus",
                         "oga", "ogg", "oma", "opus", "qcp", "ra", "rmi", "s3m", "sid", "spx", "tak", "thd", "tta",
                         "voc", "vqf", "w64", "wav", "wma", "wv", "xa", "xm" };

static std::set<QString> video_exts = { "3g2", "3gp", "3gp2", "3gpp", "amv", "asf", "avi", "bik", "crf", "dav", "divx", "drc", "dv", "dvr-ms"
                             "evo", "f4v", "flv", "gvi", "gxf", "iso",
                             "m1v", "m2v", "m2t", "m2ts", "m4v", "mkv", "mov",
                             "mp2", "mp2v", "mp4", "mp4v", "mpe", "mpeg", "mpeg1",
                             "mpeg2", "mpeg4", "mpg", "mpv2", "mts", "mtv", "mxf", "mxg", "nsv", "nuv",
                             "ogg", "ogm", "ogv", "ogx", "ps",
                             "rec", "rm", "rmvb", "rpl", "thp", "tod", "ts", "tts", "txd", "vob", "vro",
                             "webm", "wm", "wmv", "wtv", "xesc" };

bool is_ext_match(const QString& ext)
{
    return video_exts.find(ext) != video_exts.end() ||
        audio_exts.find(ext) != audio_exts.end();
}


Library::Library(QSettings* settings, const QString& root_path)
    : m_settings(settings), m_root_path(root_path)
{
    m_block_notifications = true;

    m_root = new LibraryItem();
    LibraryItem* dir_item = scan_folder(m_root_path);
    if (dir_item != nullptr)
        m_root->append_child(dir_item);

    m_block_notifications = false;
}

Library::~Library()
{
    delete m_root;
}

File* Library::load_file(const QString& path)
{
    File* file = new File(this, path);
    QFileInfo info(path);
    QString json_file = info.absolutePath() + "/" + info.completeBaseName() + ".user.json";

    QFile in_file(json_file);
    if (!in_file.open(QIODevice::ReadOnly))
        return file;

    QByteArray text = in_file.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(text));

    QJsonObject json = doc.object();

    if (json.contains("player_time") && json["player_time"].isDouble())
        file->set_player_time(json["player_time"].toInt());

    if (json.contains("length") && json["length"].isDouble())
        file->set_length(json["length"].toInt());

    if (json.contains("clips") && json["clips"].isArray())
    {
        QJsonArray json_clips = json["clips"].toArray();
        for (int i = 0; i < json_clips.size(); ++i)
        {
            Clip* clip = new Clip();
            QString text1;
            QString text2;
            QJsonObject json_clip = json_clips[i].toObject();
            if (json_clip.contains("added") && json_clip["added"].isDouble())
                clip->set_adding_time(json_clip["added"].toInt());
            if (json_clip.contains("begin") && json_clip["begin"].isDouble())
                clip->set_begin(json_clip["begin"].toInt());
            if (json_clip.contains("end") && json_clip["end"].isDouble())
                clip->set_end(json_clip["end"].toInt());
            if (json_clip.contains("level") && json_clip["level"].isDouble())
                clip->set_level(json_clip["level"].toDouble());
            if (json_clip.contains("favorite") && json_clip["favorite"].isBool())
                clip->set_favorite(json_clip["favorite"].toBool());
            if (json_clip.contains("repeated") && json_clip["repeated"].isDouble())
                clip->set_rep_time(json_clip["repeated"].toInteger());
            if (json_clip.contains("repeats") && json_clip["repeats"].isArray())
            {
                QJsonArray repeats_arr = json_clip["repeats"].toArray();
                std::vector<std::time_t> repeats;
                repeats.reserve(repeats_arr.size());
                for (QJsonValue value : repeats_arr)
                {
                    if (value.isDouble())
                        repeats.push_back(value.toDouble());
                }
                clip->set_repeats(std::move(repeats));
            }
            if (json_clip.contains("text1") && json_clip["text1"].isString())
                text1 = json_clip["text1"].toString();
            if (json_clip.contains("text2") && json_clip["text2"].isString())
                text2 = json_clip["text2"].toString();
            clip->set_subtitles({ text1, text2 });
            file->add_clip(clip);
        }
    }
    return file;
}

void Library::save_file(const File* file) const
{
    QJsonObject json;
    QJsonArray json_clips;

    json["player_time"] = file->get_player_time();
    json["length"] = file->get_length();

    for (const Clip* clip : file->get_clips())
    {
        const std::vector<std::time_t>& repeats = clip->get_repeats();
        QJsonArray repeats_arr;
        std::copy(repeats.begin(), repeats.end(), std::back_inserter(repeats_arr));

        QJsonObject json_clip;
        if(clip->get_adding_time() != 0)
            json_clip["added"] = clip->get_adding_time();
        json_clip["begin"] = clip->get_begin();
        json_clip["end"] = clip->get_end();
        json_clip["level"] = clip->get_level();
        if (clip->is_favorite())
            json_clip["favorite"] = clip->is_favorite();
        json_clip["repeated"] = clip->get_rep_time();
        if (!repeats_arr.isEmpty())
            json_clip["repeats"] = repeats_arr;
        json_clip["text1"] = clip->get_subtitle(0);
        json_clip["text2"] = clip->get_subtitle(1);
        json_clips.append(json_clip);
    }

    json["clips"] = json_clips;

    QFileInfo info(file->get_path());
    QString json_file = info.absolutePath() + "/" + info.completeBaseName() + ".user.json";

    QFile out_file(json_file);
    if (!out_file.open(QIODevice::WriteOnly))
        return;
    out_file.write(QJsonDocument(json).toJson());
}

void Library::clip_added(Clip* clip)
{
    if (!m_block_notifications)
        m_changed_files.insert(clip->get_file());
}

void Library::clip_changed(Clip* clip)
{
    if (!m_block_notifications)
        m_changed_files.insert(clip->get_file());
}

void Library::file_changed(File* file)
{
    if (!m_block_notifications)
        m_changed_files.insert(file);
}

void Library::clip_removed(Clip* clip)
{
    if (!m_block_notifications)
    {
        emit clip_removed_sig(clip);
        m_changed_files.insert(clip->get_file());
    }
}

std::vector<Clip*> Library::get_all_clips() const
{
    std::vector<Clip*> clips;
    m_root->get_clips(clips);
    return clips;
}

std::vector<Clip*> Library::find_clips(QStringView str, int max_clips) const
{
    std::vector<Clip*> clips;
    m_root->find_clips(str, max_clips, clips);
    return clips;
}

LibraryItem* Library::get_root() const
{
    return m_root;
}

QString Library::get_root_path() const
{
    return m_root_path;
}

void Library::save()
{
    for (const File* file : m_changed_files)
    {
        save_file(file);
    }
    m_changed_files.clear();
}

LibraryItem* Library::scan_folder(const QString& path, bool is_root)
{
    QDir dir(path);
    dir.setFilter(
        QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::NoSymLinks);
    dir.setSorting(QDir::DirsFirst);

    LibraryItem* cur_item = new LibraryItem(path, nullptr);

    QFileInfoList list = dir.entryInfoList();
    std::vector<LibraryItem*> dir_items;

    for (const QFileInfo& file_info : list)
    {
        if (file_info.isDir())
        {
            LibraryItem* dir_item =
                scan_folder(file_info.canonicalFilePath(), false);
            if (dir_item != nullptr)
            {
                dir_item->set_parent(cur_item);
                dir_items.push_back(dir_item);
            }
        }
        else
        {
            QString suffix = file_info.suffix().toLower();
            if (is_ext_match(suffix))
            {
                QString abs_path = file_info.canonicalFilePath();
                QString rel_path =
                    QDir(m_root_path).relativeFilePath(abs_path);

                LibraryItem* item = new LibraryItem(abs_path, cur_item);
                item->m_file = load_file(abs_path);
                dir_items.push_back(item);
            }
        }
    }

    if (!dir_items.empty())
    {
        bool expanded = m_settings->contains("expanded/" + path);
        cur_item->expand(expanded);
        cur_item->append_childs(dir_items);
        return cur_item;
    }
    else if (is_root)
    {
        return cur_item;
    }
    else
    {
        delete cur_item;
        return nullptr;
    }
}

std::vector<File*> get_files(const  std::vector<const LibraryItem*>& items)
{
    std::set<const LibraryItem*> items_set(items.begin(), items.end());

    std::vector<File*> files;
    for (const LibraryItem* item : items)
    {
        const LibraryItem* p = item->parent();
        bool parent_selected = false;
        while (p != nullptr && !parent_selected)
        {
            if (items_set.find(p) != items_set.end())
                parent_selected = true;
            p = p->parent();
        }

        if (!parent_selected)
        {
            item->get_files(files);
        }
    }
    return files;
}

void get_expanded(LibraryItem* item, std::map<QString, bool>& map)
{
    QString path = item->get_dir_path();
    if (!path.isEmpty())
        map[item->get_dir_path()] = item->is_expanded();
    for (LibraryItem* child : item->get_children())
    {
        if (child->get_item_type() == ItemType::Folder)
            get_expanded(child, map);
    }
}
