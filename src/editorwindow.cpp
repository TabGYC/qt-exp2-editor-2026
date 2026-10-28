#include "editorwindow.h"
#include "ui_editorwindow.h"
#include "codeeditor.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFileDialog>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFontDialog>
#include <QColorDialog>
#include <QTextCursor>
#include <QTextBlock>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QGridLayout>
#include <QStyle>

EditorWindow::EditorWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::EditorWindow) {
    ui->setupUi(this);
    ui->textEdit->setFont(QFont("Monospace", 12));
    ui->textEdit->setTabStopDistance(ui->textEdit->fontMetrics().horizontalAdvance(' ') * 4);
    connect(ui->actionNew, &QAction::triggered, this, &EditorWindow::newWindow);
    connect(ui->actionOpen, &QAction::triggered, this, &EditorWindow::open);
    connect(ui->actionSave, &QAction::triggered, this, [this] { save(); });
    connect(ui->actionSaveAs, &QAction::triggered, this, [this] { saveAs(); });
    connect(ui->actionQuit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionUndo, &QAction::triggered, ui->textEdit, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, ui->textEdit, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered, ui->textEdit, &QPlainTextEdit::cut);
    connect(ui->actionCopy, &QAction::triggered, ui->textEdit, &QPlainTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered, ui->textEdit, &QPlainTextEdit::paste);
    connect(ui->actionSelectAll, &QAction::triggered, ui->textEdit, &QPlainTextEdit::selectAll);
    connect(ui->textEdit, &QPlainTextEdit::copyAvailable, ui->actionCut, &QAction::setEnabled);
    connect(ui->textEdit, &QPlainTextEdit::copyAvailable, ui->actionCopy, &QAction::setEnabled);
    connect(ui->textEdit, &QPlainTextEdit::undoAvailable, ui->actionUndo, &QAction::setEnabled);
    connect(ui->textEdit, &QPlainTextEdit::redoAvailable, ui->actionRedo, &QAction::setEnabled);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &EditorWindow::updatePaste);
    ui->actionCut->setEnabled(false); ui->actionCopy->setEnabled(false);
    ui->actionUndo->setEnabled(false); ui->actionRedo->setEnabled(false); updatePaste();
    connect(ui->textEdit->document(), &QTextDocument::modificationChanged, this, [this](bool changed) {
        setWindowModified(changed); updateTitle();
    });
    connect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged, this, &EditorWindow::updateStatus);
    connect(ui->textEdit, &QPlainTextEdit::textChanged, this, &EditorWindow::updateStatus);
    connect(ui->actionFont, &QAction::triggered, this, [this] {
        bool ok = false; QFont font = QFontDialog::getFont(&ok, ui->textEdit->font(), this);
        if (ok) ui->textEdit->setFont(font);
    });
    connect(ui->actionTextColor, &QAction::triggered, this, [this] {
        QColor color = QColorDialog::getColor(ui->textEdit->palette().color(QPalette::Text), this, "字体颜色");
        if (color.isValid()) { QPalette p = ui->textEdit->palette(); p.setColor(QPalette::Text, color); ui->textEdit->setPalette(p); }
    });
    connect(ui->actionBackground, &QAction::triggered, this, [this] {
        QColor color = QColorDialog::getColor(ui->textEdit->palette().color(QPalette::Base), this, "背景颜色");
        if (color.isValid()) { QPalette p = ui->textEdit->palette(); p.setColor(QPalette::Base, color); ui->textEdit->setPalette(p); }
    });
    connect(ui->actionWrap, &QAction::toggled, this, [this](bool enabled) {
        ui->textEdit->setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    });
    connect(ui->actionLineNumbers, &QAction::toggled, ui->textEdit, &CodeEditor::setLineNumbersVisible);
    connect(ui->actionToolbar, &QAction::toggled, ui->mainToolBar, &QToolBar::setVisible);
    connect(ui->mainToolBar, &QToolBar::visibilityChanged, ui->actionToolbar, &QAction::setChecked);
    connect(ui->actionStatusbar, &QAction::toggled, ui->statusBar, &QStatusBar::setVisible);
    connect(ui->actionFind, &QAction::triggered, this, [this] { showSearch(false); });
    connect(ui->actionReplace, &QAction::triggered, this, [this] { showSearch(true); });
    connect(ui->actionAbout, &QAction::triggered, this, [this] {
        QMessageBox::about(this, "关于文本编辑器", "Qt 简单文本编辑器\n郭毅川 2024414290411\n24软件工程4班\n支持 UTF-8 文件、查找替换、撤销重做与行号。");
    });
    ui->actionNew->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    ui->actionOpen->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->actionSave->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    ui->actionUndo->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    ui->actionRedo->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setStyleSheet("QToolBar{spacing:5px;padding:5px;background:#f2f5f9;border-bottom:1px solid #d6dce5;}"
                  "QMenuBar{padding:3px;} QStatusBar{background:#f2f5f9;}"
                  "QPlainTextEdit{border:0;padding:5px;}");
    updateTitle(); updateStatus();
}
EditorWindow::~EditorWindow() { delete ui; }
CodeEditor *EditorWindow::editor() const { return ui->textEdit; }
void EditorWindow::updatePaste() {
    const QMimeData *data = QApplication::clipboard()->mimeData();
    ui->actionPaste->setEnabled(data && data->hasText());
}
void EditorWindow::updateTitle() {
    setWindowTitle((currentFile.isEmpty() ? "未命名" : QFileInfo(currentFile).fileName()) + "[*] - 文本编辑器");
    setWindowFilePath(currentFile);
}
void EditorWindow::updateStatus() {
    QTextCursor c = ui->textEdit->textCursor();
    ui->statusBar->showMessage(QString("第 %1 行，第 %2 列  |  %3 行  |  %4 字符  |  UTF-8")
        .arg(c.blockNumber() + 1).arg(c.positionInBlock() + 1)
        .arg(ui->textEdit->blockCount()).arg(ui->textEdit->toPlainText().size()));
}
void EditorWindow::newWindow() {
    auto *window = new EditorWindow;
    window->setAttribute(Qt::WA_DeleteOnClose); window->show();
}
void EditorWindow::open() {
    QString path = QFileDialog::getOpenFileName(this, "打开文本文件", {}, "文本文件 (*.txt *.cpp *.h *.md);;所有文件 (*)");
    if (path.isEmpty()) return;
    if (maybeSave()) loadFile(path);
}
bool EditorWindow::loadFile(const QString &path, bool showError) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (showError) QMessageBox::warning(this, "打开失败", file.errorString());
        return false;
    }
    // 读取成功后才替换文档，失败时保留原内容和路径。
    QByteArray bytes = file.readAll();
    if (file.error() != QFile::NoError || bytes.contains('\0')) {
        if (showError) QMessageBox::warning(this, "打开失败", "读取失败或文件包含二进制数据。");
        return false;
    }
    if (bytes.startsWith("\xEF\xBB\xBF")) bytes.remove(0, 3);
    const QString text = QString::fromUtf8(bytes);
    if (text.toUtf8() != bytes) {
        if (showError) QMessageBox::warning(this, "打开失败", "文件不是有效的 UTF-8 文本，请先转换编码。");
        return false;
    }
    ui->textEdit->setPlainText(text);
    currentFile = QFileInfo(path).absoluteFilePath();
    ui->textEdit->document()->setModified(false); updateTitle(); updateStatus(); return true;
}
bool EditorWindow::saveFile(const QString &path, bool showError) {
    // 原子保存：写入临时文件并提交成功后，才更新文件名和修改状态。
    QSaveFile file(path);
    const QByteArray bytes = ui->textEdit->toPlainText().toUtf8();
    bool ok = file.open(QIODevice::WriteOnly);
    if (ok) ok = file.write(bytes) == bytes.size();
    if (ok) ok = file.commit();
    if (!ok) {
        if (showError) QMessageBox::warning(this, "保存失败", file.errorString());
        return false;
    }
    currentFile = QFileInfo(path).absoluteFilePath();
    ui->textEdit->document()->setModified(false); updateTitle();
    ui->statusBar->showMessage("已保存 " + currentFile, 3000); return true;
}
bool EditorWindow::save() { return currentFile.isEmpty() ? saveAs() : saveFile(currentFile); }
bool EditorWindow::saveAs() {
    QString path = QFileDialog::getSaveFileName(this, "另存为", currentFile, "文本文件 (*.txt);;所有文件 (*)");
    return !path.isEmpty() && saveFile(path);
}
bool EditorWindow::maybeSave() {
    if (!ui->textEdit->document()->isModified()) return true;
    QMessageBox::StandardButton choice = QMessageBox::warning(this, "未保存的修改", "当前文档已修改，是否保存？",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (choice == QMessageBox::Save) return save();
    return choice == QMessageBox::Discard;
}
void EditorWindow::closeEvent(QCloseEvent *event) { if (maybeSave()) event->accept(); else event->ignore(); }
bool EditorWindow::findNext(const QString &text, QTextDocument::FindFlags flags) {
    if (text.isEmpty()) return false;
    QTextCursor original = ui->textEdit->textCursor();
    if (ui->textEdit->find(text, flags)) return true;
    QTextCursor cursor = original;
    cursor.movePosition(flags.testFlag(QTextDocument::FindBackward) ? QTextCursor::End : QTextCursor::Start);
    ui->textEdit->setTextCursor(cursor);
    if (ui->textEdit->find(text, flags)) return true;
    ui->textEdit->setTextCursor(original); return false;
}
void EditorWindow::replaceOne(const QString &needle, const QString &replacement, QTextDocument::FindFlags flags) {
    if (needle.isEmpty()) return;
    QTextCursor c = ui->textEdit->textCursor();
    // 重新按相同规则匹配，避免选中半个单词时绕过全词条件。
    QTextCursor start(ui->textEdit->document()); start.setPosition(c.selectionStart());
    QTextCursor match = ui->textEdit->document()->find(needle, start, flags);
    if (!c.hasSelection() || match.isNull() || match.selectionStart() != c.selectionStart() ||
        match.selectionEnd() != c.selectionEnd()) { findNext(needle, flags); return; }
    c.insertText(replacement); ui->textEdit->setTextCursor(c); findNext(needle, flags);
}
int EditorWindow::replaceAll(const QString &needle, const QString &replacement, QTextDocument::FindFlags flags) {
    if (needle.isEmpty()) return 0;
    // 从替换末尾继续，避免 replacement 含 needle 时反复匹配刚插入的文本。
    flags &= ~QTextDocument::FindBackward;
    QTextCursor cursor(ui->textEdit->document()); cursor.beginEditBlock(); int count = 0;
    while (true) {
        QTextCursor match = ui->textEdit->document()->find(needle, cursor, flags);
        if (match.isNull()) break;
        match.insertText(replacement); cursor.setPosition(match.position()); ++count;
    }
    cursor.endEditBlock(); return count;
}
void EditorWindow::showSearch(bool replace) {
    if (!searchDialog) {
        searchDialog = new QDialog(this); searchDialog->setObjectName("searchDialog");
        searchDialog->setModal(false); searchDialog->setMinimumWidth(460);
        auto *layout = new QGridLayout(searchDialog);
        findEdit = new QLineEdit(searchDialog); findEdit->setObjectName("findEdit");
        replaceEdit = new QLineEdit(searchDialog); replaceEdit->setObjectName("replaceEdit");
        auto *replaceLabel = new QLabel("替换为", searchDialog); replaceLabel->setObjectName("replaceLabel");
        caseBox = new QCheckBox("区分大小写", searchDialog);
        wholeBox = new QCheckBox("全词匹配", searchDialog);
        auto *findButton = new QPushButton("查找下一个", searchDialog); findButton->setObjectName("findNextButton");
        auto *replaceButton = new QPushButton("替换", searchDialog); replaceButton->setObjectName("replaceOneButton");
        auto *allButton = new QPushButton("全部替换", searchDialog); allButton->setObjectName("replaceAllButton");
        auto *closeButton = new QPushButton("关闭", searchDialog);
        // Return 只由输入框的 returnPressed 分发，避免默认按钮再次触发查找。
        for (QPushButton *button : {findButton, replaceButton, allButton, closeButton})
            button->setAutoDefault(false);
        searchStatus = new QLabel(searchDialog); searchStatus->setObjectName("searchStatus");
        layout->addWidget(new QLabel("查找内容", searchDialog), 0, 0); layout->addWidget(findEdit, 0, 1, 1, 3);
        layout->addWidget(replaceLabel, 1, 0); layout->addWidget(replaceEdit, 1, 1, 1, 3);
        layout->addWidget(caseBox, 2, 1); layout->addWidget(wholeBox, 2, 2);
        layout->addWidget(findButton, 3, 0); layout->addWidget(replaceButton, 3, 1);
        layout->addWidget(allButton, 3, 2); layout->addWidget(closeButton, 3, 3);
        layout->addWidget(searchStatus, 4, 0, 1, 4);
        auto flags = [this] {
            QTextDocument::FindFlags f;
            if (caseBox->isChecked()) f |= QTextDocument::FindCaseSensitively;
            if (wholeBox->isChecked()) f |= QTextDocument::FindWholeWords;
            return f;
        };
        connect(findButton, &QPushButton::clicked, this, [this, flags] {
            bool found = findNext(findEdit->text(), flags());
            searchStatus->setText(findEdit->text().isEmpty() ? "请输入查找内容" : found ? "已找到，连续查找会循环到文首" : "未找到匹配内容");
        });
        connect(findEdit, &QLineEdit::returnPressed, findButton, &QPushButton::click);
        connect(replaceButton, &QPushButton::clicked, this, [this, flags] {
            replaceOne(findEdit->text(), replaceEdit->text(), flags());
            searchStatus->setText(findEdit->text().isEmpty() ? "请输入查找内容" : "已处理当前匹配并查找下一处");
        });
        connect(allButton, &QPushButton::clicked, this, [this, flags] {
            int count = replaceAll(findEdit->text(), replaceEdit->text(), flags());
            searchStatus->setText(findEdit->text().isEmpty() ? "请输入查找内容" : QString("共替换 %1 处").arg(count));
        });
        connect(closeButton, &QPushButton::clicked, searchDialog, &QDialog::hide);
    }
    searchDialog->setWindowTitle(replace ? "替换" : "查找");
    replaceEdit->setVisible(replace);
    searchDialog->findChild<QLabel *>("replaceLabel")->setVisible(replace);
    searchDialog->findChild<QPushButton *>("replaceOneButton")->setVisible(replace);
    searchDialog->findChild<QPushButton *>("replaceAllButton")->setVisible(replace);
    searchStatus->clear(); searchDialog->show(); searchDialog->raise(); searchDialog->activateWindow(); findEdit->setFocus();
}
