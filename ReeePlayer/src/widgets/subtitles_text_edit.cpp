#include "pch.h"
#include "subtitles_text_edit.h"

SubtitlesTextEdit::SubtitlesTextEdit(QWidget * parent)
    : QPlainTextEdit(parent)
{
    installEventFilter(this);
}

void SubtitlesTextEdit::focusInEvent(QFocusEvent * e)
{
    QPlainTextEdit::focusInEvent(e);
    emit focusIn();
}

void SubtitlesTextEdit::focusOutEvent(QFocusEvent * e)
{
    QPlainTextEdit::focusOutEvent(e);
    emit focusOut();
}

void SubtitlesTextEdit::keyPressEvent(QKeyEvent* e)
{
    QPlainTextEdit::keyPressEvent(e);
}

bool SubtitlesTextEdit::eventFilter(QObject* object, QEvent* event)
{
    bool res = false;
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride)
    {
        QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
        bool ctrl_lr = key_event->modifiers() == Qt::ControlModifier &&
            (key_event->key() == Qt::Key_Left || key_event->key() == Qt::Key_Right);
        if (ctrl_lr)
            res = true;
    }

    return res;
}
