#include "PageRegistry.h"

#include "pages/AboutPage.h"
#include "pages/CanBusPage.h"
#include "pages/CescToolPage.h"
#include "pages/CommunicationPage.h"
#include "pages/MdfViewerPage.h"
#include "pages/PlotPage.h"
#include "pages/ProtocolEditorPage.h"
#include "pages/SettingsPage.h"

#include <QObject>

const QList<PageDescriptor> &pageDescriptors()
{
    static const QList<PageDescriptor> descriptors{
        {
            PageId::Communication,
            QObject::tr("Communication"),
            QStringLiteral(":/icons/navigation/connection.svg"),
            false,
            [](AppContext *context, QWidget *parent) {
                return new CommunicationPage(context, parent);
            }
        },
        {
            PageId::ProtocolEditor,
            QObject::tr("Protocol Editor"),
            QStringLiteral(":/icons/navigation/protocol.svg"),
            false,
            [](AppContext *context, QWidget *parent) {
                return new ProtocolEditorPage(context, parent);
            }
        },
        {
            PageId::Plot,
            QObject::tr("Plot"),
            QStringLiteral(":/icons/navigation/plot.svg"),
            false,
            [](AppContext *context, QWidget *parent) {
                return new PlotPage(context, parent);
            }
        },
        {
            PageId::CanBus,
            QObject::tr("CAN Bus"),
            QStringLiteral(":/icons/navigation/can_bus.svg"),
            false,
            [](AppContext *context, QWidget *parent) {
                return new CanBusPage(context, parent);
            }
        },
        {
            PageId::MdfViewer,
            QObject::tr("MDF Viewer"),
            QStringLiteral(":/icons/navigation/mdf_viewer.svg"),
            false,
            [](AppContext *context, QWidget *parent) {
                return new MdfViewerPage(context, parent);
            }
        },
        {
            PageId::CescTool,
            QStringLiteral("CESC Tool"),
            QStringLiteral(":/icons/navigation/cesc_tool.svg"),
            false,
            [](AppContext *context, QWidget *parent) {
                return new CescToolPage(context, parent);
            }
        },
        {
            PageId::About,
            QObject::tr("关于"),
            QStringLiteral(":/icons/navigation/about.svg"),
            true,
            [](AppContext *context, QWidget *parent) {
                return new AboutPage(context, parent);
            }
        },
        {
            PageId::Settings,
            QObject::tr("设置"),
            QStringLiteral(":/icons/navigation/settings.svg"),
            true,
            [](AppContext *context, QWidget *parent) {
                return new SettingsPage(context, parent);
            }
        }
    };
    return descriptors;
}

const PageDescriptor *findPageDescriptor(const PageId id)
{
    const auto &descriptors = pageDescriptors();
    for (const PageDescriptor &descriptor : descriptors) {
        if (descriptor.id == id) {
            return &descriptor;
        }
    }
    return nullptr;
}
