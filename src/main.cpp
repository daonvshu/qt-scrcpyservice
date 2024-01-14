#include <qapplication.h>

#include "qlogcollector.h"

#include "app.h"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    logcollector::styleConfig
            .consoleApp()
            .ide_clion(false)
            .wordWrap(300)
            ;
    logcollector::QLogCollector::instance().registerLog();

    App app;
    app.show();

    return a.exec();
}