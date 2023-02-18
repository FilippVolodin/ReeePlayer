#ifndef LIBRARY_H
#define LIBRARY_H

#include "library_item.h"

class Clip;
class File;

namespace srs
{
    class ICardFactory;
}

class Library : public QObject
{
    Q_OBJECT
public:
    Library(QSettings*, const QString& root_path);
    ~Library();

    void load(const srs::ICardFactory*);

    void save();
    void save(const File*);
    void save(const Clip*);

    std::vector<Clip*> get_all_clips() const;
    //std::vector<Clip*> find_clips(QStringView, int max = 0) const;

    LibraryItem* get_root() const;
    QString get_root_path() const;

signals:
    void clip_removed_sig(Clip*);

private:
    LibraryItem* scan_folder(const QString& path, bool is_root, const srs::ICardFactory*);

    LibraryItem* m_root;
    QString m_root_path;
    QSettings* m_settings;

    std::set<const File*> m_changed_files;
    bool m_block_notifications = false;
};

std::vector<File*> get_files(const std::vector<const LibraryItem*>&);
std::vector<const LibraryItem*> get_disjoint_items(const std::vector<const LibraryItem*>& items);
void get_expanded(LibraryItem* item, std::map<QString, bool>& map);

#endif // !LIBRARY_H
