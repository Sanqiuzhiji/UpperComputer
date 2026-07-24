#include <QApplication>
#include <QFont>

#include "app/AppContext.h"
#include "window/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("UpperComputer"));
    QApplication::setOrganizationName(QStringLiteral("UpperComputer"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPointSize(10);
    application.setFont(font);

    AppContext context;
    MainWindow window(&context);
    window.show();

    return QApplication::exec();
}
