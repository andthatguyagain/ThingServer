#include <QCoreApplication>
#include "thingserver.h"


int main(int argc, char *argv[])
    {
        QCoreApplication a(argc, argv);

        ThingServer server(DEFAULT_PORT);

        return a.exec();
    }
