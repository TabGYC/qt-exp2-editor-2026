#pragma once
#include <QPlainTextEdit>
class CodeEditor;
class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent *) override;
private:
    CodeEditor *editor;
};
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);
    int lineNumberWidth() const;
    void paintLineNumbers(QPaintEvent *event);
    void setLineNumbersVisible(bool visible);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
private:
    LineNumberArea *area;
    bool showNumbers = true;
    void updateMargin();
};
