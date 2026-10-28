#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QAction>
#include <QClipboard>
#include <QMessageBox>
#include <QTimer>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QToolBar>
#include <QLabel>
#include "editorwindow.h"
#include "codeeditor.h"

class EditorTest : public QObject {
    Q_OBJECT
    void capture(QWidget *widget, const QString &name) {
        const QString path = qEnvironmentVariable("EVIDENCE_DIR");
        if (!path.isEmpty()) { QDir().mkpath(path); widget->grab().save(path + "/" + name + ".png"); }
    }
private slots:
    void utf8SaveAndSaveAs() {
        QTemporaryDir dir; EditorWindow w;
        w.editor()->insertPlainText("郭毅川 Qt 实验\nHello world\n");
        QVERIFY(w.isWindowModified());
        QVERIFY(w.saveFile(dir.filePath("a.txt"), false));
        QVERIFY(!w.isWindowModified());
        QFile a(dir.filePath("a.txt")); QVERIFY(a.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(a.readAll()), w.editor()->toPlainText());
        w.editor()->insertPlainText("追加");
        QVERIFY(w.saveFile(dir.filePath("b.txt"), false));
        QCOMPARE(w.fileName(), dir.filePath("b.txt"));
        EditorWindow read; QVERIFY(read.loadFile(dir.filePath("b.txt"), false));
        QCOMPARE(read.editor()->toPlainText(), w.editor()->toPlainText());
    }
    void failedOpenPreservesDocument() {
        QTemporaryDir dir; EditorWindow w;
        w.editor()->insertPlainText("保留原文");
        QVERIFY(w.saveFile(dir.filePath("original.txt"), false));
        w.editor()->insertPlainText("未保存修改");
        const QString content = w.editor()->toPlainText(), name = w.fileName();
        QVERIFY(!w.loadFile(dir.filePath("missing.txt"), false));
        QCOMPARE(w.editor()->toPlainText(), content); QCOMPARE(w.fileName(), name);
        QVERIFY(w.isWindowModified());
    }
    void rejectBinaryAndBadUtf8() {
        QTemporaryDir dir; EditorWindow w; w.editor()->insertPlainText("original");
        for (QByteArray bytes : {QByteArray("a\0b",3), QByteArray("\xff\xfe",2)}) {
            QFile f(dir.filePath("bad.txt")); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(bytes); f.close();
            QVERIFY(!w.loadFile(f.fileName(), false)); QCOMPARE(w.editor()->toPlainText(), QString("original"));
        }
    }
    void failedSaveRetainsDirtyState() {
        QTemporaryDir dir; EditorWindow w; w.editor()->insertPlainText("unsaved");
        QVERIFY(!w.saveFile(dir.filePath("missing/fail.txt"), false));
        QVERIFY(w.isWindowModified()); QVERIFY(w.fileName().isEmpty());
    }
    void bomAndEmptyFile() {
        QTemporaryDir dir; QFile f(dir.filePath("bom.txt")); QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray::fromHex("efbbbf") + QString("中文").toUtf8()); f.close();
        EditorWindow w; QVERIFY(w.loadFile(f.fileName(), false)); QCOMPARE(w.editor()->toPlainText(), QString("中文"));
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate)); f.close();
        QVERIFY(w.loadFile(f.fileName(), false)); QVERIFY(w.editor()->toPlainText().isEmpty());
    }
    void findWrapAndNotFound() {
        EditorWindow w; w.editor()->setPlainText("Qt qt Qt");
        QTextCursor c(w.editor()->document()); w.editor()->setTextCursor(c);
        QVERIFY(w.findNext("Qt", QTextDocument::FindCaseSensitively)); QCOMPARE(w.editor()->textCursor().selectionStart(), 0);
        QVERIFY(w.findNext("Qt", QTextDocument::FindCaseSensitively)); QCOMPARE(w.editor()->textCursor().selectionStart(), 6);
        QVERIFY(w.findNext("Qt", QTextDocument::FindCaseSensitively)); QCOMPARE(w.editor()->textCursor().selectionStart(), 0);
        QVERIFY(!w.findNext("absent")); QCOMPARE(w.editor()->textCursor().selectionStart(), 0);
        QVERIFY(!w.findNext(""));
    }
    void replaceAllAndUndo() {
        EditorWindow w; w.editor()->setPlainText("a a");
        QCOMPARE(w.replaceAll("a", "aa"), 2); QCOMPARE(w.editor()->toPlainText(), QString("aa aa"));
        w.editor()->undo(); QCOMPARE(w.editor()->toPlainText(), QString("a a"));
        w.editor()->redo(); QCOMPARE(w.editor()->toPlainText(), QString("aa aa"));
        QCOMPARE(w.replaceAll("", "x"), 0);
        QCOMPARE(w.replaceAll("aa", ""), 2); QCOMPARE(w.editor()->toPlainText(), QString(" "));
    }
    void caseAndWholeWord() {
        EditorWindow w; w.editor()->setPlainText("cat scatter CAT cat");
        QCOMPARE(w.replaceAll("cat", "dog", QTextDocument::FindWholeWords | QTextDocument::FindCaseSensitively), 2);
        QCOMPARE(w.editor()->toPlainText(), QString("dog scatter CAT dog"));
    }
    void actionAvailability() {
        EditorWindow w;
        auto action = [&](const char *name) { return w.findChild<QAction *>(name); };
        QVERIFY(!action("actionCopy")->isEnabled()); QVERIFY(!action("actionCut")->isEnabled());
        QVERIFY(!action("actionUndo")->isEnabled()); QVERIFY(!action("actionRedo")->isEnabled());
        w.editor()->insertPlainText("abc"); QVERIFY(action("actionUndo")->isEnabled());
        w.editor()->selectAll(); QVERIFY(action("actionCopy")->isEnabled()); QVERIFY(action("actionCut")->isEnabled());
        action("actionCopy")->trigger(); QCOMPARE(QApplication::clipboard()->text(), QString("abc"));
        QVERIFY(action("actionPaste")->isEnabled()); action("actionCut")->trigger(); QVERIFY(w.editor()->toPlainText().isEmpty());
        action("actionPaste")->trigger(); QCOMPARE(w.editor()->toPlainText(), QString("abc"));
        action("actionUndo")->trigger(); QVERIFY(action("actionRedo")->isEnabled());
    }
    void cancelUnsavedClose() {
        EditorWindow w; w.show(); w.editor()->insertPlainText("重要内容 未保存");
        QTimer::singleShot(80, [&] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (box) { capture(box, "editor-unsaved"); box->button(QMessageBox::Cancel)->click(); }
        });
        QVERIFY(!w.close()); QVERIFY(w.isVisible()); QVERIFY(w.isWindowModified());
        QTimer::singleShot(0, [] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) box->button(QMessageBox::Discard)->click();
        });
        QVERIFY(w.close());
    }
    void cancelSaveAsKeepsDocument() {
        EditorWindow w; w.editor()->insertPlainText("not saved");
        QTimer::singleShot(0, [] {
            if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) dialog->reject();
        });
        w.findChild<QAction *>("actionSaveAs")->trigger();
        QVERIFY(w.isWindowModified()); QVERIFY(w.fileName().isEmpty());
    }
    void searchDialogEnterOnce() {
        EditorWindow w; w.editor()->setPlainText("Qt Qt Qt"); w.showSearch(false);
        auto *find = w.findChild<QLineEdit *>("findEdit"); find->setText("Qt");
        QTest::keyClick(find, Qt::Key_Return);
        QCOMPARE(w.editor()->textCursor().selectionStart(), 0);
    }
    void screenshotsAndViewControls() {
        EditorWindow w; w.show();
        w.editor()->setPlainText("Qt 应用程序开发 实验二\n郭毅川 2024414290411\n24软件工程4班\n\n文件读写：UTF-8 中文文本\n编辑：撤销、重做、剪切、复制、粘贴\n查找示例：Qt Widgets 与 Qt Designer\n替换示例：Qt 帮助构建桌面程序\n\n行号随滚动同步更新，自动换行可由菜单控制。");
        w.editor()->moveCursor(QTextCursor::Start); QTest::qWait(50); capture(&w, "editor-main");
        w.showSearch(true);
        w.findChild<QLineEdit *>("findEdit")->setText("Qt"); w.findChild<QLineEdit *>("replaceEdit")->setText("Qt6");
        QTest::mouseClick(w.findChild<QPushButton *>("replaceAllButton"), Qt::LeftButton);
        QVERIFY(w.findChild<QLabel *>("searchStatus")->text().contains("4"));
        capture(&w, "editor-replaced"); capture(w.findChild<QDialog *>("searchDialog"), "editor-replace-dialog");
        w.findChild<QLineEdit *>("findEdit")->setText("不存在的内容");
        QTest::mouseClick(w.findChild<QPushButton *>("findNextButton"), Qt::LeftButton);
        QCOMPARE(w.findChild<QLabel *>("searchStatus")->text(), QString("未找到匹配内容"));
        capture(w.findChild<QDialog *>("searchDialog"), "editor-not-found"); w.findChild<QDialog *>("searchDialog")->hide();
        w.findChild<QAction *>("actionWrap")->setChecked(false); QCOMPARE(w.editor()->lineWrapMode(), QPlainTextEdit::NoWrap);
        w.findChild<QAction *>("actionLineNumbers")->setChecked(false); QCOMPARE(w.editor()->lineNumberWidth(), 0);
        w.findChild<QAction *>("actionLineNumbers")->setChecked(true); QVERIFY(w.editor()->lineNumberWidth() > 0);
        w.findChild<QAction *>("actionToolbar")->setChecked(false); QVERIFY(w.findChild<QToolBar *>()->isHidden());
        w.findChild<QAction *>("actionStatusbar")->setChecked(false); QVERIFY(w.statusBar()->isHidden());
        QTimer::singleShot(50, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) { capture(box, "editor-about"); box->accept(); }
        });
        w.findChild<QAction *>("actionAbout")->trigger();
    }
    void multipleWindows() {
        EditorWindow w; w.show();
        int before = 0; for (QWidget *p : QApplication::topLevelWidgets()) if (qobject_cast<EditorWindow *>(p)) ++before;
        w.findChild<QAction *>("actionNew")->trigger();
        int after = 0;
        for (QWidget *p : QApplication::topLevelWidgets()) if (qobject_cast<EditorWindow *>(p)) ++after;
        QCOMPARE(after, before + 1);
        for (QWidget *p : QApplication::topLevelWidgets()) if (p != &w && qobject_cast<EditorWindow *>(p)) p->close();
    }
};
QTEST_MAIN(EditorTest)
#include "test_editor.moc"
