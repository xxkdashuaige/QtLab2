#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("简单文本编辑器"));
    MainWindow window;
    window.show();
    return app.exec();
}
