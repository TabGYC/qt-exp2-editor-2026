#pragma once
#include <QMainWindow>
#include <QTextDocument>
class CodeEditor;
class QDialog;
class QLineEdit;
class QCheckBox;
class QLabel;
namespace Ui { class EditorWindow; }
class EditorWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow();
    CodeEditor *editor() const;
    bool loadFile(const QString &path, bool showError = true);
    bool saveFile(const QString &path, bool showError = true);
    bool maybeSave();
    QString fileName() const { return currentFile; }
    bool findNext(const QString &text, QTextDocument::FindFlags flags = {});
    int replaceAll(const QString &needle, const QString &replacement, QTextDocument::FindFlags flags = {});
    void showSearch(bool replace);
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    Ui::EditorWindow *ui;
    QString currentFile;
    QDialog *searchDialog = nullptr;
    QLineEdit *findEdit = nullptr, *replaceEdit = nullptr;
    QCheckBox *caseBox = nullptr, *wholeBox = nullptr;
    QLabel *searchStatus = nullptr;
    void newWindow();
    void open();
    bool save();
    bool saveAs();
    void updateTitle();
    void updateStatus();
    void updatePaste();
    void replaceOne(const QString &needle, const QString &replacement, QTextDocument::FindFlags flags);
};
