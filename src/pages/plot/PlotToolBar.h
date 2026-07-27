#pragma once

#include <QFrame>

class AppContext;
class QToolButton;

class PlotToolBar final : public QFrame
{
    Q_OBJECT

public:
    explicit PlotToolBar(AppContext *context, QWidget *parent = nullptr);
    [[nodiscard]] bool editMode() const;
    void setPagePaused(bool paused);

signals:
    void addPageRequested();
    void addPlotRequested();
    void editModeChanged(bool enabled);
    void pauseChanged(bool paused);
    void resetRequested();
    void autoYRequested();

private:
    QToolButton *button(
        const QString &objectName,
        const QString &text,
        const QString &toolTip,
        const QString &iconPath);
    void refreshIcons();

    AppContext *m_context{};
    QToolButton *m_addPage{};
    QToolButton *m_addPlot{};
    QToolButton *m_edit{};
    QToolButton *m_pause{};
    QToolButton *m_reset{};
    QToolButton *m_autoY{};
};
