#include "PlotPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

#include <cmath>

namespace {
class CurvePreview final : public QWidget
{
public:
    explicit CurvePreview(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(230);
        setProperty("plotSurface", true);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), palette().color(QPalette::Base));

        painter.setPen(QPen(palette().color(QPalette::Mid), 1));
        constexpr int grid = 40;
        for (int x = 0; x < width(); x += grid) painter.drawLine(x, 0, x, height());
        for (int y = 0; y < height(); y += grid) painter.drawLine(0, y, width(), y);

        const QList<QColor> colors{QColor("#28A9E0"), QColor("#45D483"), QColor("#FF9F43")};
        for (int curve = 0; curve < colors.size(); ++curve) {
            QPainterPath path;
            for (int x = 0; x < width(); ++x) {
                const double phase = static_cast<double>(x) / 55.0 + curve * 0.9;
                const double value = std::sin(phase) * (42.0 - curve * 7.0)
                    + std::cos(phase * 0.37) * 9.0;
                const double y = height() * (0.34 + curve * 0.17) - value;
                x == 0 ? path.moveTo(x, y) : path.lineTo(x, y);
            }
            painter.setPen(QPen(colors.at(curve), 1.7));
            painter.drawPath(path);
        }
    }
};

QFrame *card(QWidget *parent, const QString &title)
{
    auto *frame = new QFrame(parent);
    frame->setProperty("card", true);
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(12, 10, 12, 12);
    layout->setSpacing(8);
    auto *label = new QLabel(title, frame);
    label->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(label);
    return frame;
}

void addControl(QVBoxLayout *layout, const QString &name, QWidget *control, QWidget *parent)
{
    auto *label = new QLabel(name, parent);
    label->setProperty("muted", true);
    layout->addWidget(label);
    layout->addWidget(control);
}
}

PlotPage::PlotPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 14);
    root->setSpacing(10);

    auto *toolbar = new QFrame(this);
    toolbar->setProperty("card", true);
    toolbar->setFixedHeight(50);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(10, 5, 10, 5);
    auto *workspace = new QComboBox(toolbar);
    workspace->addItems({QStringLiteral("ws1"), QStringLiteral("ws2")});
    auto *idle = new QPushButton(tr("idle"), toolbar);
    idle->setCheckable(true);
    idle->setChecked(true);
    auto *ready = new QPushButton(tr("ready"), toolbar);
    ready->setCheckable(true);
    auto *run = new QPushButton(tr("run"), toolbar);
    run->setCheckable(true);
    run->setProperty("accent", true);
    auto *power = new QCheckBox(tr("ON"), toolbar);
    auto *value = new QLabel(QStringLiteral("12.48 V   2.31 A   1480 rpm"), toolbar);
    value->setObjectName(QStringLiteral("telemetryValue"));
    toolbarLayout->addWidget(new QLabel(tr("Workspace"), toolbar));
    toolbarLayout->addWidget(workspace);
    toolbarLayout->addSpacing(12);
    toolbarLayout->addWidget(idle);
    toolbarLayout->addWidget(ready);
    toolbarLayout->addWidget(run);
    toolbarLayout->addWidget(power);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(value);
    root->addWidget(toolbar);

    auto *horizontal = new QSplitter(Qt::Horizontal, this);

    auto *controls = card(horizontal, tr("Controls"));
    controls->setMinimumWidth(185);
    controls->setMaximumWidth(260);
    auto *controlLayout = qobject_cast<QVBoxLayout *>(controls->layout());
    auto *mode = new QComboBox(controls);
    mode->addItems({tr("Position"), tr("Velocity"), tr("Current")});
    addControl(controlLayout, tr("Control mode"), mode, controls);
    auto *slider = new QSlider(Qt::Horizontal, controls);
    slider->setRange(-100, 100);
    slider->setValue(24);
    addControl(controlLayout, tr("Command"), slider, controls);
    auto *input = new QLineEdit(QStringLiteral("24.00"), controls);
    addControl(controlLayout, tr("Target value"), input, controls);
    auto *enable = new QCheckBox(tr("Enable output"), controls);
    controlLayout->addWidget(enable);
    auto *apply = new QPushButton(tr("Apply"), controls);
    apply->setProperty("accent", true);
    controlLayout->addWidget(apply);
    controlLayout->addSpacing(10);
    auto *metrics = new QGridLayout;
    const QStringList metricNames{tr("Voltage"), tr("Current"), tr("Speed"), tr("Torque")};
    const QStringList metricValues{QStringLiteral("12.48 V"), QStringLiteral("2.31 A"),
                                   QStringLiteral("1480 rpm"), QStringLiteral("0.82 Nm")};
    for (int row = 0; row < metricNames.size(); ++row) {
        auto *name = new QLabel(metricNames.at(row), controls);
        name->setProperty("muted", true);
        metrics->addWidget(name, row, 0);
        metrics->addWidget(new QLabel(metricValues.at(row), controls), row, 1);
    }
    controlLayout->addLayout(metrics);
    controlLayout->addStretch();
    horizontal->addWidget(controls);

    auto *center = new QSplitter(Qt::Vertical, horizontal);
    auto *curveCard = card(center, tr("Realtime curves · 10 s window"));
    auto *curveLayout = qobject_cast<QVBoxLayout *>(curveCard->layout());
    curveLayout->addWidget(new CurvePreview(curveCard), 1);
    center->addWidget(curveCard);

    auto *terminalCard = card(center, tr("Terminal"));
    auto *terminalLayout = qobject_cast<QVBoxLayout *>(terminalCard->layout());
    auto *terminalTools = new QHBoxLayout;
    terminalTools->addWidget(new QToolButton(terminalCard));
    auto *clear = new QPushButton(tr("Clear"), terminalCard);
    terminalTools->addWidget(new QLabel(tr("UTF-8   RX   TX   ANSI"), terminalCard));
    terminalTools->addStretch();
    terminalTools->addWidget(clear);
    auto *terminal = new QPlainTextEdit(terminalCard);
    terminal->setReadOnly(true);
    terminal->setPlainText(
        QStringLiteral("[18:52:01.084] Virtual source initialized\n"
                       "[18:52:01.101] CH1  12.480 V\n"
                       "[18:52:01.117] CH2   2.310 A\n"
                       "[18:52:01.134] Sampling at 100 Hz"));
    terminalLayout->addLayout(terminalTools);
    terminalLayout->addWidget(terminal);
    connect(clear, &QPushButton::clicked, terminal, &QPlainTextEdit::clear);
    center->addWidget(terminalCard);
    center->setSizes({520, 190});
    horizontal->addWidget(center);

    auto *channels = card(horizontal, tr("Channels"));
    channels->setMinimumWidth(210);
    channels->setMaximumWidth(300);
    auto *channelLayout = qobject_cast<QVBoxLayout *>(channels->layout());
    const QList<QPair<QString, QString>> channelData{
        {QStringLiteral("●  Motor speed"), QStringLiteral("1480 rpm")},
        {QStringLiteral("●  Bus voltage"), QStringLiteral("12.48 V")},
        {QStringLiteral("●  Phase current"), QStringLiteral("2.31 A")},
        {QStringLiteral("●  Temperature"), QStringLiteral("36.2 °C")},
        {QStringLiteral("●  Torque"), QStringLiteral("0.82 Nm")}
    };
    const QStringList colors{QStringLiteral("#28A9E0"), QStringLiteral("#45D483"),
                             QStringLiteral("#FF9F43"), QStringLiteral("#D875F3"),
                             QStringLiteral("#E95D68")};
    for (qsizetype index = 0; index < channelData.size(); ++index) {
        auto *row = new QFrame(channels);
        row->setProperty("channelRow", true);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(5, 6, 5, 6);
        auto *check = new QCheckBox(channelData.at(index).first, row);
        check->setChecked(index < 3);
        check->setStyleSheet(QStringLiteral("color:%1;").arg(colors.at(index)));
        auto *reading = new QLabel(channelData.at(index).second, row);
        rowLayout->addWidget(check, 1);
        rowLayout->addWidget(reading);
        channelLayout->addWidget(row);
    }
    channelLayout->addStretch();
    horizontal->addWidget(channels);
    horizontal->setSizes({210, 850, 250});

    root->addWidget(horizontal, 1);
}
