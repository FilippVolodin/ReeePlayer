#ifndef DOMITEM_H
#define DOMITEM_H

#include "clip_storage.h"

enum class ItemType { Folder, File };

class Library;

class LibraryItem
{
    friend Library;
public:
    LibraryItem() = default;
    LibraryItem(const QString& path, LibraryItem* parent = nullptr);
    ~LibraryItem();
    LibraryItem* child(int i);
    const LibraryItem* child(int i) const;
    LibraryItem* parent();
    const LibraryItem* parent() const;
    void set_parent(LibraryItem*);
    int row() const;
    int num_children() const;

    File* get_file();
    const File* get_file() const;
    QString get_name() const;
    QString get_dir_path() const;

    void get_files(std::vector<File*>&) const;

    const std::vector<LibraryItem*>& get_children() const;

    int get_id() const;
    std::vector<int> get_ids() const;

    ItemType get_item_type() const;

    QVariant data(int column, int role) const;

    Qt::ItemFlags flags(int column) const;

    void expand(bool);
    bool is_expanded() const;

    int get_clips_count() const;
    void update_clips_count();

    void append_child(LibraryItem*);
    void append_childs(const std::vector<LibraryItem*>&);
    void insert_child(LibraryItem*, int pos);
    void insert_childs(const std::vector<LibraryItem*>&, int);
    void remove_childs(int row, int count);

    void get_clips(std::vector<Clip*>&);

    using FileFunc = std::function<void(File*)>;
    using ConstFileFunc = std::function<void(const File*)>;
    using ClipFunc = std::function<void(Clip*)>;
    using ConstClipFunc = std::function<void(const Clip*)>;

    void iterate_files(FileFunc cf);
    void iterate_files(ConstFileFunc cf) const;
    void iterate_clips(ClipFunc cf);
    void iterate_clips(ConstClipFunc cf) const;
private:
    void get_ids(std::vector<int>&) const;
    void update_clips_count_internal(TimePoint);

    //QHash<int, DomItem *> childItems;
    QString m_name;
    QString m_dir_path;
    std::unique_ptr<File> m_file;
    std::vector<LibraryItem*> m_child_items;
    LibraryItem *m_parent = nullptr;
    Qt::CheckState m_checked = Qt::Unchecked;
    ItemType m_item_type;
    int m_id = -1;
    int m_clips_count = 0;
    int m_due_count = 0;
    bool m_expanded;
    //Qt::ItemFlags m_item_flags;
};

#endif