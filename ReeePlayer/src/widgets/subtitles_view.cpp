#include "pch.h"
#include "subtitles_view.h"
#include "subs_combobox.h"

constexpr int OFFSET_STEP = 500;

SubtitlesView::SubtitlesView(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    QToolBar* toolbar = new QToolBar();
    toolbar->setIconSize(QSize(18, 18));
    
    QBoxLayout* lo = dynamic_cast<QBoxLayout*>(layout());
    if (lo != nullptr)
    {
        lo->insertWidget(0, toolbar);
    }
    else
    {
        lo->addWidget(toolbar);
    }

    toolbar->addAction(ui.btnShowSubtitle);
    toolbar->addAction(ui.btnShowSubtitles);

    m_insert_group_sep = toolbar->addSeparator();
    toolbar->addAction(ui.btnInsertLeft);
    toolbar->addAction(ui.btnInsertRight);

    m_subs_files_group_sep = toolbar->addSeparator();

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolbar->addWidget(spacer);

    m_sb_sync = new QSpinBox(this);
    m_sb_sync->setMinimum(-1000000);
    m_sb_sync->setMaximum(1000000);
    m_sb_sync->setSingleStep(100);
    m_sb_sync->setGroupSeparatorShown(true);
    m_sb_sync->setSuffix(" ms");
    m_sb_sync->setFocusPolicy(Qt::FocusPolicy(Qt::ClickFocus /*| 0x4*/));
    m_sb_sync_action = toolbar->addWidget(m_sb_sync);
    m_sync_group_sep = toolbar->addSeparator();

    m_cmb_subs = new SubsComboBox(this);
    m_cmb_subs->setFocusPolicy(Qt::NoFocus);
    m_cmb_subs_action = toolbar->addWidget(m_cmb_subs);

    connect(ui.btnShowSubtitles, &QAction::toggled, [this](bool show)
    {
        set_show_always(show);
        emit on_show_always(show);
    });

    connect(ui.btnShowSubtitle, &QAction::toggled, [this](bool show)
    {
        set_show_once(show);
        emit on_show_once(show);
    });

    connect(m_sb_sync, &QSpinBox::valueChanged, this, &SubtitlesView::on_sb_sync_valueChanged, Qt::QueuedConnection);

    connect(ui.btnSpeedUp, &QAction::triggered, [this]()
    {
        m_offset += OFFSET_STEP;
        update_offset_buttons_text();
    });

    connect(ui.btnSlowDown, &QAction::triggered, [this]()
    {
        m_offset -= OFFSET_STEP;
        update_offset_buttons_text();
    });

    connect(ui.btnInsertLeft, &QAction::triggered, [this]()
    {
        --m_inserted_left;
        emit on_insert_clicked(m_inserted_left);
    });

    connect(ui.btnInsertRight, &QAction::triggered, [this]()
    {
        ++m_inserted_right;
        emit on_insert_clicked(m_inserted_right);
    });

    connect(ui.edtSubtitles, &SubtitlesTextEdit::focusIn, [this]()
    {
        update_background();
    });

    connect(ui.edtSubtitles, &SubtitlesTextEdit::focusOut, [this]()
    {
        update_background();
    });

    connect(m_cmb_subs, &QComboBox::currentIndexChanged,
        [this](int index)
        {
            emit on_file_changed(index);
        });

    m_is_visible = ui.btnShowSubtitle->isChecked();
    update_offset_buttons_text();
}

SubtitlesView::~SubtitlesView()
{
}

QString SubtitlesView::get_text() const
{
    if (m_is_visible)
        return ui.edtSubtitles->toPlainText();
    else
        return m_text;
}

void SubtitlesView::set_text(const QString& text)
{
    m_text = text;
    ui.edtSubtitles->setToolTip(m_text);
    update_text_visibility();
}

void SubtitlesView::clear()
{
    m_text.clear();
    ui.edtSubtitles->clear();
}

void SubtitlesView::set_editable(bool value)
{
    m_is_editable = value;
    ui.edtSubtitles->setReadOnly(!m_is_visible || !m_is_editable);
    update_background();
}

bool SubtitlesView::is_showed_once() const
{
    return ui.btnShowSubtitle->isChecked();
}

bool SubtitlesView::is_showed_always() const
{
    return ui.btnShowSubtitles->isChecked();
}

void SubtitlesView::set_show_once(bool show)
{
    ui.btnShowSubtitle->blockSignals(true);

    if (m_is_visible && !show)
        m_text = get_text();
    bool old_visibility = m_is_visible;
    m_is_visible = show;
    ui.btnShowSubtitle->setChecked(show);
    if (old_visibility != m_is_visible)
        update_text_visibility();

    ui.btnShowSubtitle->blockSignals(false);
}

void SubtitlesView::set_show_always(bool show)
{
    ui.btnShowSubtitle->blockSignals(true);
    ui.btnShowSubtitles->blockSignals(true);

    if (m_is_visible && !show)
        m_text = get_text();
    bool old_visibility = m_is_visible;
    m_is_visible = show;
    ui.btnShowSubtitle->setChecked(show);
    ui.btnShowSubtitles->setChecked(show);
    if (old_visibility != m_is_visible)
        update_text_visibility();

    ui.btnShowSubtitles->blockSignals(false);
    ui.btnShowSubtitle->blockSignals(false);
}

void SubtitlesView::next()
{
    set_show_always(is_showed_always());
}

int SubtitlesView::get_offset() const
{
    return m_offset;
}

void SubtitlesView::set_show_offset_buttons(bool show)
{
    ui.btnSlowDown->setVisible(show);
    ui.btnSpeedUp->setVisible(show);
    m_sb_sync_action->setVisible(show);
    m_sync_group_sep->setVisible(show);
}

void SubtitlesView::set_show_insert_buttons(bool show)
{
    ui.btnInsertLeft->setVisible(show);
    ui.btnInsertRight->setVisible(show);
    m_insert_group_sep->setVisible(show);
}

void SubtitlesView::set_show_subs_files(bool show)
{
    //ui.lblSubs->setVisible(show);
    m_cmb_subs_action->setVisible(show);
    m_subs_files_group_sep->setVisible(show);
}

int SubtitlesView::get_inserted_left() const
{
    return m_inserted_left;
}

int SubtitlesView::get_inserted_right() const
{
    return m_inserted_right;
}

void SubtitlesView::reset_insert_counters()
{
    m_inserted_left = 0;
    m_inserted_right = 0;
    ui.btnInsertLeft->setToolTip(QString());
    ui.btnInsertRight->setToolTip(QString());
}

void SubtitlesView::set_insert_left_button_tip(const QString& tip)
{
    ui.btnInsertLeft->setToolTip(tip);
}

void SubtitlesView::set_insert_right_button_tip(const QString& tip)
{
    ui.btnInsertRight->setToolTip(tip);
}

void SubtitlesView::set_focus()
{
    ui.edtSubtitles->setFocus();
}

QComboBox* SubtitlesView::get_combobox()
{
    return m_cmb_subs;
}

const QPlainTextEdit* SubtitlesView::get_edit() const
{
    return ui.edtSubtitles;
}

void SubtitlesView::update_text_visibility()
{
    if (m_is_visible)
    {
        ui.edtSubtitles->setPlainText(m_text);
    }
    else
    {
        if (!m_text.isEmpty())
            ui.edtSubtitles->setPlainText("...");
        else
            ui.edtSubtitles->clear();
    }
    ui.edtSubtitles->setReadOnly(!m_is_visible || !m_is_editable);
    update_background();
}

void SubtitlesView::update_background()
{
    QPalette p = ui.edtSubtitles->palette();
    QColor color(0xFAFAFA);
    bool can_focused = false;
    if (ui.edtSubtitles->isEnabled() && !ui.edtSubtitles->isReadOnly())
    {
        if (ui.edtSubtitles->hasFocus())
            color = QColor(0xCD, 0xF8, 0xC5);
        else
            color = Qt::white;
        can_focused = true;
    }
    p.setColor(QPalette::Active, QPalette::Base, color);
    p.setColor(QPalette::Inactive, QPalette::Base, color);
    ui.edtSubtitles->setPalette(p);
    ui.edtSubtitles->setFocusPolicy(
        can_focused ? Qt::FocusPolicy::StrongFocus : Qt::ClickFocus);
}

void SubtitlesView::update_offset_buttons_text()
{
    QString tip = QString("%1 ms (now: %2)").
        arg(OFFSET_STEP).arg(m_offset);
    ui.btnSpeedUp->setToolTip("+" + tip);
    ui.btnSlowDown->setToolTip("-" + tip);
}

void SubtitlesView::on_sb_sync_valueChanged(int value)
{
    m_offset = value;
    m_sb_sync->findChild<QLineEdit*>()->deselect();
}
