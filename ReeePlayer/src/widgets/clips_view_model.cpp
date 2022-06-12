#include "pch.h"
#include "clips_view_model.h"
#include "models/repetition_model.h"
#include "models/library.h"

ClipModel::ClipModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int ClipModel::rowCount(const QModelIndex&) const
{
    return m_clips ? m_clips->size() : 0;
}

int ClipModel::columnCount(const QModelIndex&) const
{
    return m_show_path ? 6 : 5;
}

QVariant ClipModel::data(const QModelIndex & index, int role) const
{
    int row = index.row();
    int col = index.column();
    const Clip* clip = (*m_clips)[row];

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (col)
        {
        case 0: return clip->is_favorite() ? QString("★") : QString();
        case 1: return clip->get_subtitle(0);
        case 2: return clip->get_subtitle(1);
        case 3:
        {
            if (clip->get_adding_time() != 0)
            {
                QDateTime time = QDateTime::fromSecsSinceEpoch(clip->get_adding_time());
                return time.date(); // .toString("dd.MM.yyyy");
            }
            else
                return QVariant();
        }
        case 4: return clip->get_repeats().size();
        case 5:
        {
            const Library* library = clip->get_file()->get_library();
            QString root_path = library->get_root_path();
            QString file_path = clip->get_file()->get_path();
            if (file_path.startsWith(root_path))
            {
                int pos = root_path.length();
                if (file_path[pos] == '/' || file_path[pos] == '\\')
                    pos++;
                return file_path.mid(pos);
            }
            else
                return file_path;
        }
        default: return QVariant();
        }
    }
    };
    return QVariant();
}

QVariant ClipModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal)
    {
        if (role == Qt::DisplayRole)
        {
            switch (section)
            {
            case 0: return tr("F");
            case 1: return tr("Text 1");
            case 2: return tr("Text 2");
            case 3: return tr("Added");
            case 4: return tr("Reps");
            case 5: return tr("Path");
            default: break;
            }
        }
    }
    else if (orientation == Qt::Vertical)
    {
        if (role == Qt::DisplayRole)
            return section + 1;
    }
    return QVariant();
}

void ClipModel::set_show_path(bool show_path)
{
    m_show_path = show_path;
}

Clip* ClipModel::get_clip(int row) const
{
    if (row >= 0 && row < m_clips->size())
        return (*m_clips)[row];
    else
        return nullptr;
}

void ClipModel::set_clips(ClipsPtr clips)
{
    beginResetModel();
    m_clips = clips;
    endResetModel();
}
