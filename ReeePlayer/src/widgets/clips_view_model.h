#ifndef CLIP_VIEW_MODEL_H
#define CLIP_VIEW_MODEL_H

class Clip;

class ClipModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ClipModel(QObject *parent);
    int rowCount(const QModelIndex &parent = QModelIndex())
        const override;
    int columnCount(const QModelIndex &parent = QModelIndex())
        const override;
    QVariant data(const QModelIndex &, int = Qt::DisplayRole)
        const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role)
        const override;

    void set_show_path(bool);

    Clip* get_clip(int row) const;
    void set_clips(std::vector<Clip*>);

private:
    bool m_show_path = true;
    std::vector<Clip*> m_clips;
};

#endif 
