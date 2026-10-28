#include <QApplication>
#include "editorwindow.h"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("QtTextEditor");
    EditorWindow window;
    if (argc > 1) window.loadFile(QString::fromLocal8Bit(argv[1]));
    window.show();
    return app.exec();
}
