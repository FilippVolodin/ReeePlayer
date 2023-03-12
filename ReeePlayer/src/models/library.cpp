#include "pch.h"
#include "library.h"
#include "clip_storage.h"
#include <srs.h>

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
}

Library::~Library()
{
    delete m_root;
}

void Library::load(const srs::IFactory* card_factory)
{
    m_block_notifications = true;

    m_root = new LibraryItem();
    LibraryItem* dir_item = scan_folder(m_root_path, true, card_factory);
    if (dir_item)
        m_root->append_child(dir_item);

    m_block_notifications = false;

    std::set<const File*> changed_files;
    auto set_uid = [&changed_files](Clip* clip)
    {
        if (clip->get_uid().isEmpty())
        {
            clip->generate_uid();
            changed_files.insert(clip->get_file());
        }
    };
    m_root->iterate_clips(set_uid);

    m_root->update_clips_count();
    
    for (const File* file : changed_files)
        save(file);
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

void Library::save(const File* file)
{
    m_changed_files.insert(file);
}

void Library::save(const Clip* clip)
{
    m_changed_files.insert(clip->get_file());
}

LibraryItem* Library::scan_folder(const QString& path, bool is_root, const srs::IFactory* card_factory)
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
                scan_folder(file_info.canonicalFilePath(), false, card_factory);
            if (dir_item)
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
                item->m_file = load_file(this, abs_path, card_factory);
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

std::vector<File*> get_files(const std::vector<const LibraryItem*>& items)
{
    std::set<const LibraryItem*> items_set(items.begin(), items.end());

    std::vector<File*> files;
    for (const LibraryItem* item : items)
    {
        const LibraryItem* p = item->parent();
        bool parent_selected = false;
        while (p && !parent_selected)
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

std::vector<LibraryItem*> get_disjoint_items(const std::vector<LibraryItem*>& items)
{
    std::set<LibraryItem*> items_set(items.begin(), items.end());

    std::vector<LibraryItem*> disjoint_items;
    for (LibraryItem* item : items)
    {
        LibraryItem* p = item->parent();
        bool parent_selected = false;
        while (p && !parent_selected)
        {
            if (items_set.find(p) != items_set.end())
                parent_selected = true;
            p = p->parent();
        }

        if (!parent_selected)
        {
            disjoint_items.push_back(item);
        }
    }
    return disjoint_items;
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
