#include "pch.h"
#include "subtitles_text_edit.h"

SubtitlesTextEdit::SubtitlesTextEdit(QWidget * parent)
    : QPlainTextEdit(parent)
{
    installEventFilter(this);

    QPalette p = palette();
    //m_default_base_color_name = p.base().color().name();
    m_default_base_color = p.color(QPalette::Base); // p.base().color();
    m_default_border_color = p.window().color().darker(140).lighter(108);


    connect(this, &SubtitlesTextEdit::focusIn, [this]()
    {
        update_background();
    });

    connect(this, &SubtitlesTextEdit::focusOut, [this]()
    {
        update_background();
    });
}

SubtitlesTextEdit::~SubtitlesTextEdit()
{
}

void SubtitlesTextEdit::update_background()
{
    QColor base_color = QPalette().color(QPalette::Disabled, QPalette::Base);
    QColor border_color = m_default_border_color;
    //QString base_color_name = QPalette().color(QPalette::Disabled, QPalette::Base).name();
    bool can_focused = false;
    if (isEnabled() && !isReadOnly())
    {
        border_color = hasFocus() ? QColor("#54a0ec") : QPalette().color(QPalette::AlternateBase);
        base_color = m_default_base_color;
        can_focused = true;
    }

    QString style_sheet = QString(".SubtitlesTextEdit {background-color: %1; border: 2px solid %2;}")
        .arg(base_color.name()).arg(border_color.name());
    setStyleSheet(style_sheet);

    setFocusPolicy(
        can_focused ? Qt::FocusPolicy::StrongFocus : Qt::ClickFocus);
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

bool SubtitlesTextEdit::eventFilter(QObject*, QEvent* event)
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

void SubtitlesTextEdit::paintEvent(QPaintEvent* e)
{
    QPlainTextEdit::paintEvent(e);
    return;
    parentWidget();
    QPainter painter(viewport());
    painter.save();

    QPen thePen(Qt::red);
    thePen.setCosmetic(false);
    painter.setPen(thePen);
    QRect rec = rect();
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
    painter.restore();
}
