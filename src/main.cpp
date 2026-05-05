#include <QApplication>
#include <QDebug>
#include <cstdlib>
#include <exception>
#include "mainwindow.h"
#include "startuplogger.h"

int main(int argc, char *argv[]) {
    StartupLogger::initialize(argc, argv);
    StartupLogger::installQtMessageHandler();
    StartupLogger::installCrashHandler();
    StartupLogger::info("main() entered");

    try {
        QApplication app(argc, argv);
        qInfo() << "QApplication created";

        app.setApplicationName("FileSwitch");

        MainWindow w;
        qInfo() << "MainWindow constructed";

        w.show();
        qInfo() << "MainWindow shown";

        const int exitCode = app.exec();
        qInfo() << "Application event loop exited with code" << exitCode;
        return exitCode;
    } catch (const std::exception &exception) {
        StartupLogger::critical(QString("Unhandled std::exception in main(): %1").arg(exception.what()));
    } catch (...) {
        StartupLogger::critical("Unhandled unknown exception in main()");
    }

    return EXIT_FAILURE;
}
