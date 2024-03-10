#pragma once

namespace pm
{

    class SubtitlesTextEdit : public QPlainTextEdit
    {
        Q_OBJECT
    public:
        SubtitlesTextEdit(QWidget* parent = nullptr);
        ~SubtitlesTextEdit();

        void update_background();
    signals:
        void focusIn();
        void focusOut();

    protected:
        void focusInEvent(QFocusEvent* e) override;
        void focusOutEvent(QFocusEvent* e) override;

        void keyPressEvent(QKeyEvent* e) override;

        bool eventFilter(QObject* object, QEvent* event);

        void paintEvent(QPaintEvent* e) override;
    private:
        QColor m_default_base_color;
        QString m_default_base_color_name;
        QColor m_default_disable_base_color;
        //QString m_default_style_sheet;
        QColor m_default_border_color;

    };
}
