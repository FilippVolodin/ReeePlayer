#ifndef SUBTITLES_TEXT_EDIT_H
#define SUBTITLES_TEXT_EDIT_H

class SubtitlesTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    SubtitlesTextEdit(QWidget *parent = nullptr);

signals:
    void focusIn();
    void focusOut();

protected:
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

    void keyPressEvent(QKeyEvent* e) override;

    bool eventFilter(QObject* object, QEvent* event);

};

#endif