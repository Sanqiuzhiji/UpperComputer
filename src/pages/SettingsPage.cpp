#include "SettingsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

SettingsPage::SettingsPage(ThemeManager *themeManager, QWidget *parent)
    : QWidget(parent),
      m_themeManager(themeManager)
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(28, 24, 28, 28);
    rootLayout->setSpacing(14);
    auto *title = new QLabel(tr("设置"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    rootLayout->addWidget(title);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto *content = new QWidget(scrollArea);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 8, 0);
    contentLayout->setSpacing(10);

    m_themeCombo = createOptions({tr("深色"), tr("浅色")});
    m_themeCombo->setCurrentIndex(m_themeManager->mode() == ThemeMode::Dark ? 0 : 1);
    contentLayout->addWidget(createSettingCard(
        tr("外观主题"), tr("切换应用程序的深色或浅色主题。"), m_themeCombo));
    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, [this](const int index) {
        emit themeModeRequested(index == 0 ? ThemeMode::Dark : ThemeMode::Light);
    });
    connect(m_themeManager, &ThemeManager::themeChanged, this, [this](const ThemeMode mode) {
        m_themeCombo->blockSignals(true);
        m_themeCombo->setCurrentIndex(mode == ThemeMode::Dark ? 0 : 1);
        m_themeCombo->blockSignals(false);
    });

    auto *msaa = createOptions({tr("关闭"), tr("2 倍"), tr("4 倍"), tr("8 倍")});
    contentLayout->addWidget(createSettingCard(
        tr("多重采样抗锯齿"), tr("用于后续曲线和三维视图的抗锯齿等级。"), msaa));
    auto *fps = createOptions({tr("30 帧/秒"), tr("60 帧/秒"), tr("120 帧/秒")});
    fps->setCurrentIndex(1);
    contentLayout->addWidget(createSettingCard(
        tr("绘图刷新率"), tr("模拟曲线和实时曲线的目标刷新率。"), fps));
    auto *userCard = new QCheckBox(tr("显示"), this);
    userCard->setChecked(true);
    contentLayout->addWidget(createSettingCard(
        tr("用户信息卡片"), tr("在导航栏顶部显示软件信息。"), userCard));
    connect(userCard, &QCheckBox::toggled, this, &SettingsPage::userCardVisibilityChanged);

    auto *renderMode = createOptions({tr("原生"), tr("光栅"), tr("OpenGL")});
    contentLayout->addWidget(createSettingCard(
        tr("窗口绘制模式"), tr("窗口绘制后端选项，当前仅保留界面。"), renderMode));
    auto *visualEffect = createOptions({tr("标准"), tr("云母"), tr("亚克力")});
    contentLayout->addWidget(createSettingCard(
        tr("窗口视觉效果"), tr("Windows 11 窗口背景效果，当前仅保留界面。"), visualEffect));
    auto *navigation = createOptions({tr("展开"), tr("紧凑"), tr("自动")});
    contentLayout->addWidget(createSettingCard(
        tr("导航栏模式"), tr("选择展开、紧凑或根据窗口宽度自动调整。"), navigation));
    connect(navigation, &QComboBox::currentTextChanged,
            this, &SettingsPage::navigationModeChanged);
    auto *transition = createOptions({tr("无动画"), tr("淡入"), tr("滑动")});
    contentLayout->addWidget(createSettingCard(
        tr("页面切换方式"), tr("页面切换动画将在后续版本实现。"), transition));

    const auto unavailable = [this](const QString &name, QComboBox *combo) {
        connect(combo, &QComboBox::activated, this, [this, name](int) {
            emit unavailableSettingRequested(name);
        });
    };
    unavailable(tr("多重采样抗锯齿"), msaa);
    unavailable(tr("绘图刷新率"), fps);
    unavailable(tr("窗口绘制模式"), renderMode);
    unavailable(tr("窗口视觉效果"), visualEffect);
    unavailable(tr("页面切换方式"), transition);

    contentLayout->addStretch();
    scrollArea->setWidget(content);
    rootLayout->addWidget(scrollArea, 1);
}

QWidget *SettingsPage::createSettingCard(const QString &title,
                                         const QString &description,
                                         QWidget *control)
{
    auto *card = new QFrame(this);
    card->setProperty("card", true);
    card->setMinimumHeight(76);
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(18, 12, 18, 12);
    auto *textLayout = new QVBoxLayout;
    textLayout->setSpacing(3);
    auto *titleLabel = new QLabel(title, card);
    QFont font = titleLabel->font();
    font.setBold(true);
    titleLabel->setFont(font);
    auto *descriptionLabel = new QLabel(description, card);
    descriptionLabel->setProperty("muted", true);
    descriptionLabel->setWordWrap(true);
    textLayout->addWidget(titleLabel);
    textLayout->addWidget(descriptionLabel);
    control->setParent(card);
    control->setMinimumWidth(150);
    layout->addLayout(textLayout, 1);
    layout->addWidget(control, 0, Qt::AlignVCenter);
    return card;
}

QComboBox *SettingsPage::createOptions(const QStringList &options)
{
    auto *combo = new QComboBox(this);
    combo->addItems(options);
    return combo;
}
