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
    std::vector<Clip*> find_clips(QStringView, int max) const;

    LibraryItem* get_root() const;
    QString get_root_path() const;

    LibraryItem* get_dir_files(const QString& path);

    int refresh_clips_count();

signals:

    void clip_removed_sig(Clip*);
private:
    LibraryItem* scan_folder(const QString& path, bool is_root = true);
    //void get_file_set(const LibraryItem* item, QSet<QString>& files) const;
    //void get_file_map(const LibraryItem* item,
    //    QMap<int, QString>& files) const;
    //void get_map(const LibraryItem* item,
    //    QMap<int, const LibraryItem*>& map) const;
    //void get_max_id(const LibraryItem* item, int& max) const;

    //void fill_maps(const LibraryItem* item);
    //void fill_maps(const std::vector<LibraryItem*> item);
    //void clear_maps(const LibraryItem* item);

    File* load_file(const QString&);
    void save_file(const File*) const;

    QDomDocument m_document;
    LibraryItem* m_root;
    QSet<QString> m_file_set;
    QMap<int, QString> m_file_map;
    QMap<int, const LibraryItem*> m_map;
    int m_max_id = -1;
    QString m_root_path;
    QSettings* m_settings;

    std::set<File*> m_changed_files;
    bool m_block_notifications = false;
};

std::vector<File*> get_files(std::vector<const LibraryItem*>&);
void get_expanded(LibraryItem* item, std::map<QString, bool>& map);

#endif // !LIBRARY_H
