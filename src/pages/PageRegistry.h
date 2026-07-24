#pragma once

#include <QList>
#include <QString>

#include <functional>

#include "pages/PageId.h"

class AppContext;
class QWidget;

struct PageDescriptor {
    PageId id;
    QString title;
    QString iconPath;
    bool alignBottom;
    std::function<QWidget *(AppContext *, QWidget *)> factory;
};

[[nodiscard]] const QList<PageDescriptor> &pageDescriptors();
[[nodiscard]] const PageDescriptor *findPageDescriptor(PageId id);
