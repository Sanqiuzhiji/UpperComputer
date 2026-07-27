#pragma once

#include <QDialog>

#include "pages/plot/PlotTypes.h"

class ChannelDataHub;
class AppContext;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

class PlotChannelDialog final : public QDialog
{
    Q_OBJECT

public:
    PlotChannelDialog(
        AppContext *context,
        const QList<PlotChannelStyle> &styles,
        QWidget *parent = nullptr);
    [[nodiscard]] QList<PlotChannelStyle> styles() const;

private:
    void populate();
    void filter(const QString &text);
    void chooseColor(QTreeWidgetItem *item, int column);
    [[nodiscard]] QColor defaultColor(int index) const;

    ChannelDataHub *m_hub{};
    QList<PlotChannelStyle> m_initialStyles;
    QLineEdit *m_search{};
    QTreeWidget *m_list{};
};
