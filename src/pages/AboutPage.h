#pragma once

#include <QWidget>

class AppContext;

class AboutPage final : public QWidget
{
    Q_OBJECT

public:
    explicit AboutPage(AppContext *context, QWidget *parent = nullptr);
};
