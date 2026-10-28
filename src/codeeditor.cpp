#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QEvent>

LineNumberArea::LineNumberArea(CodeEditor *e) : QWidget(e), editor(e) {}
QSize LineNumberArea::sizeHint() const { return QSize(editor->lineNumberWidth(), 0); }
void LineNumberArea::paintEvent(QPaintEvent *e) { editor->paintLineNumbers(e); }
CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent), area(new LineNumberArea(this)) {
    connect(this, &QPlainTextEdit::blockCountChanged, this, [this] { updateMargin(); });
    connect(this, &QPlainTextEdit::updateRequest, this, [this](QRect rect, int dy) {
        if (dy) area->scroll(0, dy); else area->update(0, rect.y(), area->width(), rect.height());
        if (rect.contains(viewport()->rect())) updateMargin();
    });
    updateMargin();
}
int CodeEditor::lineNumberWidth() const {
    return showNumbers ? 14 + fontMetrics().horizontalAdvance('9') * QString::number(blockCount()).size() : 0;
}
void CodeEditor::updateMargin() {
    setViewportMargins(lineNumberWidth(), 0, 0, 0);
    const QRect r = contentsRect();
    area->setGeometry(r.left(), r.top(), lineNumberWidth(), r.height());
    area->update();
}
void CodeEditor::setLineNumbersVisible(bool value) { showNumbers = value; area->setVisible(value); updateMargin(); }
void CodeEditor::resizeEvent(QResizeEvent *event) { QPlainTextEdit::resizeEvent(event); updateMargin(); }
void CodeEditor::changeEvent(QEvent *event) {
    QPlainTextEdit::changeEvent(event);
    if (event->type() == QEvent::FontChange) updateMargin();
}
void CodeEditor::paintLineNumbers(QPaintEvent *event) {
    QPainter painter(area); painter.fillRect(event->rect(), QColor("#edf1f5"));
    QTextBlock block = firstVisibleBlock(); int number = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    while (block.isValid() && top <= event->rect().bottom()) {
        int bottom = top + qRound(blockBoundingRect(block).height());
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(QColor("#68778a"));
            painter.drawText(0, top, area->width() - 6, fontMetrics().height(), Qt::AlignRight, QString::number(number + 1));
        }
        block = block.next(); top = bottom; ++number;
    }
}
