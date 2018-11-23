#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication::setApplicationName("Silouette");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("Hunleth");
    QApplication::setOrganizationDomain("hunleth.com");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
