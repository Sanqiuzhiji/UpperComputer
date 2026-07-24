#pragma once

#include <QWidget>

class AppContext;

class PlotPage final : public QWidget
{
    Q_OBJECT

public:
    explicit PlotPage(AppContext *context, QWidget *parent = nullptr);
};
