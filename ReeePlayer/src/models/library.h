#ifndef LIBRARY_H
#define LIBRARY_H

#include "library_item.h"

class Clip;
class File;

class Library : public QObject
{
    Q_OBJECT
public:
    Library(QSettings*, const QString& root_path);
    ~Library();

    void clip_added(Clip*);
    void clip_removed(Clip*);
    void clip_changed(Clip*);
    void file_changed(File*);

    void save();

    std::vector<Clip*> get_all_clips() const;
    //std::vector<Clip*> find_clips(QStringView, int max = 0) const;

    LibraryItem* get_root() const;
    QString get_root_path() const;

signals:
    void clip_removed_sig(Clip*);

private:
    LibraryItem* scan_folder(const QString& path, bool is_root = true);

    File* load_file(const QString&);
    void save_file(const File*) const;

    LibraryItem* m_root;
    QString m_root_path;
    QSettings* m_settings;

    std::set<File*> m_changed_files;
    bool m_block_notifications = false;
};

std::vector<File*> get_files(const std::vector<const LibraryItem*>&);
std::vector<const LibraryItem*> get_disjoint_items(const std::vector<const LibraryItem*>& items);
void get_expanded(LibraryItem* item, std::map<QString, bool>& map);

#endif // !LIBRARY_H
