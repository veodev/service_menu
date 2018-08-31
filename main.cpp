#include "widget.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator translator;
    translator.load(":/translations/russian.qm", ".");
    a.installTranslator(&translator);
    Widget w;
    w.show();

    return a.exec();
}
