#include "pch.h"
#include "clips_view_model.h"
#include "models/library.h"

ClipModel::ClipModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int ClipModel::rowCount(const QModelIndex&) const
{
    return m_clips ? ssize(*m_clips) : 0;
}

int ClipModel::columnCount(const QModelIndex&) const
{
    return m_show_path ? 7 : 6;
}

QVariant ClipModel::data(const QModelIndex & index, int role) const
{
    int row = index.row();
    int col = index.column();
    const Clip* clip = (*m_clips)[row];
    const ClipUserData* user_data = clip->get_user_data();

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (col)
        {
        case 0: return user_data->is_favorite ? QString("★") : QString();
        case 1: return user_data->subtitles[0];
        case 2: return user_data->subtitles[1];
        case 3:
        {
            if (clip->get_adding_time() != TimePoint(Duration::zero()))
            {
                auto time_sec = clip->get_adding_time().time_since_epoch().count();
                QDateTime time = QDateTime::fromSecsSinceEpoch(time_sec);
                return time.date();
            }
            else
                return QVariant();
        }
        case 4: return clip->get_repeats().size();
        case 5:
        {
            const srs::ICard* card = clip->get_card();
            if (card)
            {
                return get_interval_str(card->get_due_date() - now());
            }
        }
        case 6:
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
    case Qt::FontRole:
    {
        QFont font;
        if (clip->is_removed())
            font.setItalic(true);
        return font;
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
            case 5: return tr("Due");
            case 6: return tr("Path");
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

void ClipModel::clear()
{
    m_clips.reset();
}
