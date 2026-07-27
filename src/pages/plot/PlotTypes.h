#pragma once

#include <QColor>
#include <QString>

struct PlotChannelStyle
{
    QString channelId;
    bool visible{true};
    QColor color;
    qreal lineWidth{1.5};
};

enum class PlotRenderMode {
    Line,
    Points
};
