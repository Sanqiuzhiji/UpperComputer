#pragma once

#include <QMetaType>

enum class PageId {
    Plot,
    Communication,
    ProtocolEditor,
    CanBus,
    MdfViewer,
    About,
    Settings
};

Q_DECLARE_METATYPE(PageId)
