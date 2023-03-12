#ifndef LIBRARY_H
#define LIBRARY_H

#include "library_item.h"

class Clip;
class File;

namespace srs
{
    class IFactory;
}

class Library : public QObject
{
    Q_OBJECT
public:
    Library(QSettings*, const QString& root_path);
    ~Library();

    void load(const srs::IFactory*);

    void save();
    void save(const File*);
    void save(const Clip*);

    LibraryItem* get_root() const;
    QString get_root_path() const;

private:
    LibraryItem* scan_folder(const QString& path, bool is_root, const srs::IFactory*);

    LibraryItem* m_root;
    QString m_root_path;
    QSettings* m_settings;

    std::set<const File*> m_changed_files;
    bool m_block_notifications = false;
};

std::vector<File*> get_files(const std::vector<const LibraryItem*>&);
std::vector<LibraryItem*> get_disjoint_items(const std::vector<LibraryItem*>& items);
void get_expanded(LibraryItem* item, std::map<QString, bool>& map);

#endif // !LIBRARY_H
