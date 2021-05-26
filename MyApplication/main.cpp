#include "client.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Client w;
    w.setWindowTitle("My Application");
    w.show();

    return a.exec();
}
