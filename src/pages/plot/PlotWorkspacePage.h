#pragma once

#include <QWidget>

class AppContext;
class PlotCanvas;

class PlotWorkspacePage final : public QWidget
{
    Q_OBJECT

public:
    explicit PlotWorkspacePage(AppContext *context, QWidget *parent = nullptr);
    [[nodiscard]] PlotCanvas *canvas() const noexcept;
    [[nodiscard]] bool paused() const noexcept;

public slots:
    void setPaused(bool paused);

private:
    PlotCanvas *m_canvas{};
    bool m_paused{};
};
