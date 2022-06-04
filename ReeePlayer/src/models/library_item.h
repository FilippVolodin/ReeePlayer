#ifndef DOMITEM_H
#define DOMITEM_H

class File;
class Clip;

enum class ItemType { Folder, File };

class Library;

class LibraryItem
{
    friend Library;
public:
    LibraryItem() = default;
    LibraryItem(const QString& path, LibraryItem* parent = nullptr);
    ~LibraryItem();
    LibraryItem* child(int i) const;
    LibraryItem* parent() const;
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

    int get_clips_count(bool deep = false) const;
    void update_clips_count_up();

    void append_child(LibraryItem*);
    void append_childs(const std::vector<LibraryItem*>&);
    void insert_child(LibraryItem*, int pos);
    void insert_childs(const std::vector<LibraryItem*>&, int);
    void remove_childs(int row, int count);

    void get_clips(std::vector<Clip*>&);
    void find_clips(QStringView str, int max_clips, bool fav, std::vector<Clip*>&) const;
private:
    void get_ids(std::vector<int>&) const;

    //QHash<int, DomItem *> childItems;
    QString m_name;
    QString m_dir_path;
    File* m_file = nullptr;
    std::vector<LibraryItem*> m_child_items;
    LibraryItem *m_parent = nullptr;
    Qt::CheckState m_checked = Qt::Unchecked;
    ItemType m_item_type;
    int m_id = -1;
    mutable int m_clips_count = -1;
    bool m_expanded;
    //Qt::ItemFlags m_item_flags;
};

#endif
