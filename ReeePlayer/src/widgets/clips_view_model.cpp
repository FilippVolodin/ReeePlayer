#include "pch.h"
#include "clips_view_model.h"
#include "models/repetition_model.h"
#include "models/clip_storage.h"

ClipModel::ClipModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int ClipModel::rowCount(const QModelIndex&) const
{
    return m_clips.size();
}

int ClipModel::columnCount(const QModelIndex&) const
{
    return 4;
}

QVariant ClipModel::data(const QModelIndex & index, int role) const
{
    int row = index.row();
    int col = index.column();
    const Clip* clip = m_clips[row];

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (col)
        {
        case 0:
        {
            return clip->get_subtitle(0);
        }
        case 1:
        {
            return QString::number(clip->get_level());
        }
        case 2:
        {
            QDateTime time = QDateTime::fromSecsSinceEpoch(clip->get_rep_time());
            return time.toString("dd.MM.yyyy");
        }
        case 3:
        {
            BestRepInterval rep_int =
                get_repetititon_interval(clip->get_level());
            int64_t rep_time = clip->get_rep_time() + rep_int.begin;
            QDateTime time = QDateTime::fromSecsSinceEpoch(rep_time);
            return time.toString("dd.MM.yyyy");
        }
        }
    }
    };
    return QVariant();
}

QVariant ClipModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return tr("Text");
        case 1:
            return tr("Level");
        case 2:
            return tr("Date");
        case 3:
            return tr("Next Rep Date");
        default:
            break;
        }
    }
    return QVariant();
}

Clip* ClipModel::get_clip(int row) const
{
    if (row >= 0 && row < m_clips.size())
        return m_clips[row];
    else
        return nullptr;
}

void ClipModel::set_clips(std::vector<Clip*> clips)
{
    beginResetModel();
    m_clips = std::move(clips);
    endResetModel();
}
