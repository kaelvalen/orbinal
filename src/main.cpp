#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Orbinal");

    QFont font("Courier", 9);
    app.setFont(font);

    MainWindow w;
    w.show();

    return app.exec();
}
