#include <qapplication.h>

#include "app.h"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    App app;
    app.show();

    return a.exec();
}