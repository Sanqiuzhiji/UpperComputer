#pragma once

#include <QWidget>

class AppContext;

class ConnectionPage final : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionPage(AppContext *context, QWidget *parent = nullptr);
};
