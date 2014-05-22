#include "widgets.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MusM::MainWindow w;


    w.show();
    return a.exec();
}
