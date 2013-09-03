#include <QApplication>
#include "Controller.h"
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/images/appicon_ouka_alice.png"));

    qtauController c;
    c.run(); // create UI that can immediately call back, thus requires already created & inited controller

    return app.exec();
}
