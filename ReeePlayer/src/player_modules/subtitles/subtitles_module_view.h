#pragma once

#include "ui_subtitles_view.h"

namespace pm
{

    class SubtitlesView : public QWidget
    {
        Q_OBJECT

    public:
        SubtitlesView(QWidget* parent = Q_NULLPTR);
        ~SubtitlesView();

        QString get_text() const;
        void set_text(const QString&);
        void clear();

        void set_editable(bool);

        bool is_showed_once() const;
        bool is_showed_always() const;
        void set_show_once(bool);
        void set_show_always(bool);

        void next();

        int get_offset() const;

        void set_show_offset_buttons(bool);
        void set_show_insert_buttons(bool);
        void set_show_subs_files(bool);

        int get_inserted_left() const;
        int get_inserted_right() const;
        void reset_insert_counters();

        void set_insert_left_button_tip(const QString&);
        void set_insert_right_button_tip(const QString&);

        void set_focus();

        QComboBox* get_combobox();
        const QPlainTextEdit* get_edit() const;
    signals:
        void on_show_once(bool);
        void on_show_always(bool);
        void on_insert_clicked(int);
        void on_file_changed(int);
    private:
        void update_text_visibility();
        void update_background();
        void update_offset_buttons_text();

        void on_sb_sync_valueChanged(int);

        Ui::SubtitlesView ui;

        QComboBox* m_cmb_subs;
        QAction* m_cmb_subs_action;

        QSpinBox* m_sb_sync;
        QAction* m_sb_sync_action;

        QAction* m_sync_group_sep;
        QAction* m_insert_group_sep;
        QAction* m_subs_files_group_sep;

        QString m_text;
        bool m_is_visible = true;
        bool m_is_editable = false;

        int m_offset = 0;
        int m_inserted_left = 0;
        int m_inserted_right = 0;
    };
}
