#ifndef DOMMODEL_H
#define DOMMODEL_H

#include "models/library.h"

class LibraryTree : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit LibraryTree(QObject *parent = nullptr);
    ~LibraryTree();

    QVariant data(const QModelIndex &index, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
        const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void set_library(Library*);

    LibraryItem* get_item(const QModelIndex& index) const;

    void expanded(const QModelIndex &index);
    void collapsed(const QModelIndex &index);
    bool is_expanded(const QModelIndex &index);
private:
    Library* m_library = nullptr;
};

#endif
