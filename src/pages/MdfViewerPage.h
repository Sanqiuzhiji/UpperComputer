#pragma once

#include <QWidget>

class AppContext;

class MdfViewerPage final : public QWidget
{
    Q_OBJECT

public:
    explicit MdfViewerPage(AppContext *context, QWidget *parent = nullptr);
};
