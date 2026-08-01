#include <QApplication>
#include <QDir>
#include <QFont>
#include <QSettings>

#include "app/AppContext.h"
#include "window/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("UpperComputer"));
    QApplication::setOrganizationName(QStringLiteral("UpperComputer"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

#ifdef UPPERCOMPUTER_PORTABLE_BUILD
    const QString configDirectory =
        QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("config"));
    QDir().mkpath(configDirectory);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       configDirectory);
#endif

    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPointSize(10);
    application.setFont(font);

    AppContext context;
    MainWindow window(&context);
    window.show();

    return QApplication::exec();
}
