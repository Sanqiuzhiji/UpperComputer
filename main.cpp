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

    // Times New Roman supplies Latin letters and digits. Chinese glyphs
    // automatically fall back to Microsoft YaHei through the application QSS.
    QFont font(QStringLiteral("Times New Roman"));
    font.setPointSize(10);
    application.setFont(font);

    ThemeManager themeManager;
    MainWindow window(&themeManager);
    window.show();

    return QApplication::exec();
}
