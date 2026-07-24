#pragma once

#include <QWidget>

class AppContext;

class ProtocolEditorPage final : public QWidget
{
    Q_OBJECT

public:
    explicit ProtocolEditorPage(AppContext *context, QWidget *parent = nullptr);
};
