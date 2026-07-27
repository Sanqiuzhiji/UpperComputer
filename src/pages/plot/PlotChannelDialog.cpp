#include "PlotChannelDialog.h"

#include "app/AppContext.h"
#include "pages/plot/PlotTheme.h"
#include "services/ChannelDataHub.h"
#include "theme/ThemeManager.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <iterator>

namespace {
enum Column {
    VisibleColumn,
    NameColumn,
    UnitColumn,
    ColorColumn,
    WidthColumn,
    StateColumn
};
constexpr int kChannelIdRole = Qt::UserRole;
constexpr int kColorRole = Qt::UserRole + 1;
}

PlotChannelDialog::PlotChannelDialog(
    AppContext *context,
    const QList<PlotChannelStyle> &styles,
    QWidget *parent)
    : QDialog(parent),
      m_hub(context->channelDataHub()),
      m_initialStyles(styles)
{
    setObjectName(QStringLiteral("plotChannelDialog"));
    setPalette(PlotTheme::palette(context->themeManager()->mode()));
    setWindowTitle(tr("通道配置"));
    resize(720, 440);
    auto *layout = new QVBoxLayout(this);
    m_search = new QLineEdit(this);
    m_search->setObjectName(QStringLiteral("plotChannelSearch"));
    m_search->setPlaceholderText(tr("搜索通道名称、单位或 ID"));
    layout->addWidget(m_search);
    m_list = new QTreeWidget(this);
    m_list->setObjectName(QStringLiteral("plotChannelList"));
    m_list->setColumnCount(6);
    m_list->setHeaderLabels({
        tr("显示"), tr("通道"), tr("单位"),
        tr("颜色"), tr("线宽"), tr("状态")});
    m_list->header()->setSectionResizeMode(NameColumn, QHeaderView::Stretch);
    m_list->setRootIsDecorated(false);
    m_list->setAlternatingRowColors(true);
    layout->addWidget(m_list, 1);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    connect(m_search, &QLineEdit::textChanged,
            this, &PlotChannelDialog::filter);
    connect(m_list, &QTreeWidget::itemDoubleClicked,
            this, &PlotChannelDialog::chooseColor);
    connect(context->themeManager(), &ThemeManager::themeChanged,
            this, [this](const ThemeMode mode) {
                setPalette(PlotTheme::palette(mode));
                update();
            });
    populate();
}

QList<PlotChannelStyle> PlotChannelDialog::styles() const
{
    QList<PlotChannelStyle> result;
    for (int index = 0; index < m_list->topLevelItemCount(); ++index) {
        const QTreeWidgetItem *item = m_list->topLevelItem(index);
        bool ok = false;
        const qreal width = item->text(WidthColumn).toDouble(&ok);
        result.append({
            item->data(NameColumn, kChannelIdRole).toString(),
            item->checkState(VisibleColumn) == Qt::Checked,
            item->data(ColorColumn, kColorRole).value<QColor>(),
            ok ? qBound<qreal>(0.5, width, 8.0) : 1.5
        });
    }
    return result;
}

void PlotChannelDialog::populate()
{
    QHash<QString, PlotChannelStyle> configured;
    for (const PlotChannelStyle &style : m_initialStyles) {
        configured.insert(style.channelId, style);
    }
    const QList<ChannelDescriptor> channels = m_hub->channels();
    int colorIndex = 0;
    for (const ChannelDescriptor &channel : channels) {
        const PlotChannelStyle style = configured.take(channel.id);
        auto *item = new QTreeWidgetItem(m_list);
        item->setCheckState(
            VisibleColumn,
            style.channelId.isEmpty() || style.visible
                ? Qt::Checked : Qt::Unchecked);
        item->setText(NameColumn, channel.displayName);
        item->setToolTip(NameColumn, channel.id);
        item->setData(NameColumn, kChannelIdRole, channel.id);
        item->setText(UnitColumn, channel.unit);
        const QColor color = style.color.isValid()
            ? style.color : defaultColor(colorIndex++);
        item->setData(ColorColumn, kColorRole, color);
        item->setBackground(ColorColumn, color);
        item->setText(ColorColumn, color.name());
        item->setText(
            WidthColumn,
            QString::number(style.lineWidth > 0.0 ? style.lineWidth : 1.5));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setText(StateColumn, tr("已绑定"));
    }
    for (auto it = configured.cbegin(); it != configured.cend(); ++it) {
        auto *item = new QTreeWidgetItem(m_list);
        item->setCheckState(
            VisibleColumn, it->visible ? Qt::Checked : Qt::Unchecked);
        item->setText(NameColumn, it.key());
        item->setData(NameColumn, kChannelIdRole, it.key());
        const QColor color =
            it->color.isValid() ? it->color : defaultColor(colorIndex++);
        item->setData(ColorColumn, kColorRole, color);
        item->setBackground(ColorColumn, color);
        item->setText(ColorColumn, color.name());
        item->setText(WidthColumn, QString::number(it->lineWidth));
        item->setText(StateColumn, tr("未绑定"));
        item->setForeground(StateColumn, QColor(QStringLiteral("#E5484D")));
    }
}

void PlotChannelDialog::filter(const QString &text)
{
    const QString needle = text.trimmed();
    for (int index = 0; index < m_list->topLevelItemCount(); ++index) {
        QTreeWidgetItem *item = m_list->topLevelItem(index);
        const QString haystack = QStringLiteral("%1 %2 %3")
            .arg(item->text(NameColumn),
                 item->text(UnitColumn),
                 item->data(NameColumn, kChannelIdRole).toString());
        item->setHidden(
            !needle.isEmpty()
            && !haystack.contains(needle, Qt::CaseInsensitive));
    }
}

void PlotChannelDialog::chooseColor(
    QTreeWidgetItem *item, const int column)
{
    if (!item || column != ColorColumn) return;
    QColorDialog dialog(
        item->data(ColorColumn, kColorRole).value<QColor>(), this);
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    dialog.setWindowTitle(tr("选择曲线颜色"));
    if (dialog.exec() != QDialog::Accepted) return;
    const QColor selected = dialog.selectedColor();
    if (!selected.isValid()) return;
    item->setData(ColorColumn, kColorRole, selected);
    item->setBackground(ColorColumn, selected);
    item->setText(ColorColumn, selected.name());
}

QColor PlotChannelDialog::defaultColor(const int index) const
{
    static const QColor colors[]{
        QColor("#28A9E0"), QColor("#45B97C"), QColor("#F59E0B"),
        QColor("#E5484D"), QColor("#8B5CF6"), QColor("#22B8CF"),
        QColor("#F472B6"), QColor("#A3E635")};
    return colors[index % static_cast<int>(std::size(colors))];
}
