#include "pch.h"
#include "library_item.h"

LibraryItem::LibraryItem(const QString& path, LibraryItem* parent)
    : m_parent(parent)
{
    QFileInfo info(path);
    if (info.isDir())
    {
        m_item_type = ItemType::Folder;
        m_dir_path = path;
    }
    else
    {
        m_item_type = ItemType::File;
    }

    if (!info.isRoot())
        m_name = info.fileName();
    else
        m_name = path;
}

LibraryItem::~LibraryItem()
{
    qDeleteAll(m_child_items);
}

void LibraryItem::get_ids(std::vector<int>& ids) const
{
    if (get_item_type() == ItemType::File)
    {
        ids.push_back(get_id());
    }
    else if (get_item_type() == ItemType::Folder)
    {
        for (LibraryItem* item : m_child_items)
            item->get_ids(ids);
    }
}

LibraryItem* LibraryItem::parent()
{
    return m_parent;
}

const LibraryItem* LibraryItem::parent() const
{
    return m_parent;
}

void LibraryItem::set_parent(LibraryItem* parent)
{
    m_parent = parent;
}

LibraryItem* LibraryItem::child(int i)
{
    if (i < 0 || i >= m_child_items.size())
        return nullptr;

    return m_child_items[i];
}

const LibraryItem* LibraryItem::child(int i) const
{
    if (i < 0 || i >= m_child_items.size())
        return nullptr;

    return m_child_items[i];
}

int LibraryItem::row() const
{
    if (m_parent != nullptr)
    {
        auto it = std::find(
            m_parent->m_child_items.begin(),
            m_parent->m_child_items.end(), this);
        if (it == m_parent->m_child_items.end())
            return 0;
        return std::distance(m_parent->m_child_items.begin(), it);
    }
    else
        return 0;
}

int LibraryItem::num_children() const
{
    return static_cast<int>(m_child_items.size());
}

File* LibraryItem::get_file()
{
    return m_file.get();
}

const File* LibraryItem::get_file() const
{
    return m_file.get();
}

QString LibraryItem::get_name() const
{
    return m_name;
}

QString LibraryItem::get_dir_path() const
{
    return m_dir_path;
}

void LibraryItem::get_files(std::vector<File*>& files) const
{
    if (get_item_type() == ItemType::Folder)
    {
        for (const LibraryItem* child : m_child_items)
        {
            child->get_files(files);
        }
    }
    else
    {
        files.push_back(m_file.get());
    }
}

const std::vector<LibraryItem*>& LibraryItem::get_children() const
{
    return m_child_items;
}

int LibraryItem::get_id() const
{
    return m_id;
}

std::vector<int> LibraryItem::get_ids() const
{
    std::vector<int> ids;
    get_ids(ids);
    return ids;
}

ItemType LibraryItem::get_item_type() const
{
    return m_item_type;
}

QVariant LibraryItem::data(int column, int role) const
{
    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:
    {
        switch (column)
        {
        case 0:
        {
            return m_name;
        }
        case 1:
        {
            return m_clips_count;
        }
        case 2:
        {
            return m_due_count;
        }
        default:
            break;
        }
        break;
    }
    case Qt::DecorationRole:
    {
        if (column == 0)
        {
            const char* res;
            if (m_item_type == ItemType::Folder)
                res = is_expanded() ?
                ":/MainWindow/opened_folder" :
                ":/MainWindow/closed_folder";
            else if (m_item_type == ItemType::File)
                res = ":/MainWindow/video_file";
            QPixmap pixmap(res);
            return QVariant::fromValue(pixmap);
        }
        break;
    }
    }
    return QVariant();
}

Qt::ItemFlags LibraryItem::flags(int column) const
{
    Qt::ItemFlags flags;
    flags = Qt::ItemIsEnabled;
    if (column == 0)
    {
        if (m_item_type == ItemType::File)
            flags |= Qt::ItemNeverHasChildren;

        flags |= Qt::ItemIsSelectable;
    }
    else
    {
        flags |= Qt::ItemNeverHasChildren;
    }
    return flags;
}

void LibraryItem::expand(bool value)
{
    m_expanded = value;
}

bool LibraryItem::is_expanded() const
{
    return m_expanded;
}

void LibraryItem::update_clips_count()
{
    update_clips_count_internal(now());
}

void LibraryItem::update_clips_count_internal(TimePoint now)
{
    m_clips_count = 0;
    m_due_count = 0;
    if (get_item_type() == ItemType::File)
    {
        iterate_clips(
            [this, now](const Clip* clip)
            {
                if (!clip->is_removed())
                {
                    ++this->m_clips_count;
                    if (clip->get_card() != nullptr && clip->get_card()->is_due(now))
                        ++this->m_due_count;
                }
            }
        );
    }
    else if (get_item_type() == ItemType::Folder)
    {
        for (LibraryItem* item : m_child_items)
        {
            item->update_clips_count_internal(now);
            m_clips_count += item->m_clips_count;
            m_due_count += item->m_due_count;
        }
    }
}


//void LibraryItem::update_clips_count_up()
//{
//    m_cached_clips_count = update_clips_count();
//
//    LibraryItem* p = parent();
//    while (p != nullptr)
//    {
//        p->m_cached_clips_count = p->update_clips_count();
//        //if (p->get_item_type() == ItemType::File)
//        //{
//        //    p->m_cached_clips_count = m_file->get_num_clips();
//        //}
//        //else if (p->get_item_type() == ItemType::Folder)
//        //{
//        //    p->m_cached_clips_count = 0;
//        //    for (LibraryItem* item : p->m_child_items)
//        //        p->m_cached_clips_count += item->update_clips_count();
//        //}
//        p = p->parent();
//    }
//}

void LibraryItem::append_child(LibraryItem* item)
{
    item->m_parent = this;
    m_child_items.push_back(item);
}

void LibraryItem::append_childs(const std::vector<LibraryItem*>& items)
{
    for (LibraryItem* item : items)
        item->m_parent = this;

    m_child_items.reserve(m_child_items.size() + items.size());
    std::copy(items.begin(), items.end(), std::back_inserter(m_child_items));
}

void LibraryItem::insert_child(LibraryItem* item, int pos)
{
    item->m_parent = this;

    m_child_items.insert(m_child_items.begin() + pos, item);
}

void LibraryItem::insert_childs(const std::vector<LibraryItem*>& items, int pos)
{
    for (LibraryItem* item : items)
        item->m_parent = this;

    m_child_items.insert(m_child_items.begin() + pos,
        items.begin(), items.end());
}

void LibraryItem::remove_childs(int row, int count)
{
    auto begin = m_child_items.begin() + row;
    auto end = m_child_items.begin() + row + count;
    for (auto it = begin; it != end; ++it)
        delete *it;

    m_child_items.erase(begin, end);
}

void LibraryItem::get_clips(std::vector<Clip*>& clips)
{
    if (get_item_type() == ItemType::File)
    {
        auto file_clips = m_file->get_clips();
        clips.insert(clips.end(), file_clips.begin(), file_clips.end());
    }
    else if (get_item_type() == ItemType::Folder)
    {
        for (LibraryItem* item : m_child_items)
            item->get_clips(clips);
    }
}

void LibraryItem::iterate_files(FileFunc ff)
{
    if (get_item_type() == ItemType::File)
    {
        ff(m_file.get());
    }
    else if (get_item_type() == ItemType::Folder)
    {
        for (LibraryItem* item : m_child_items)
            item->iterate_files(ff);
    }
}

void LibraryItem::iterate_files(ConstFileFunc ff) const
{
    if (get_item_type() == ItemType::File)
    {
        ff(m_file.get());
    }
    else if (get_item_type() == ItemType::Folder)
    {
        for (const LibraryItem* item : m_child_items)
            item->iterate_files(ff);
    }
}

void LibraryItem::iterate_clips(ClipFunc f)
{
    if (get_item_type() == ItemType::File)
    {
        auto file_clips = m_file->get_clips();
        for (Clip* clip : file_clips)
            f(clip);
    }
    else if (get_item_type() == ItemType::Folder)
    {
        for (LibraryItem* item : m_child_items)
            item->iterate_clips(f);
    }
}

void LibraryItem::iterate_clips(ConstClipFunc f) const
{
    if (get_item_type() == ItemType::File)
    {
        auto file_clips = m_file->get_clips();
        for (Clip* clip : file_clips)
            f(clip);
    }
    else if (get_item_type() == ItemType::Folder)
    {
        for (const LibraryItem* item : m_child_items)
            item->iterate_clips(f);
    }
}