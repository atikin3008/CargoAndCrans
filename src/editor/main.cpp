#include "../include/editor/EditorWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    editor::EditorWindow window;
    window.show();
    return app.exec();
}
