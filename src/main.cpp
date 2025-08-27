#include "mainwindow.h"      // Includes the MainWindow class definition
#include <QApplication>      // Includes the QApplication class for GUI app management

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);   // Initializes the Qt application
    MainWindow window;              // Creates an instance of the main window
    window.show();                  // Displays the main window on the screen
    return app.exec();              // Starts the Qt event loop
}
