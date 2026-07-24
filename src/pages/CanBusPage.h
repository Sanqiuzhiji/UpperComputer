#pragma once

#include <QWidget>

class AppContext;

class CanBusPage final : public QWidget
{
    Q_OBJECT

public:
    explicit CanBusPage(AppContext *context, QWidget *parent = nullptr);
};
