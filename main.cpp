#include <QApplication>
#include <QFont>

#include "theme/ThemeManager.h"
#include "window/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("UpperComputer"));
    QApplication::setOrganizationName(QStringLiteral("UpperComputer"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QFont font(QStringLiteral("Segoe UI"));
    font.setPointSize(10);
    application.setFont(font);

    ThemeManager themeManager;
    MainWindow window(&themeManager);
    window.show();

    return QApplication::exec();
}
