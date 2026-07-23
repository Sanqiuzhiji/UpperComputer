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

    auto *title = new QLabel(tr("Settings"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);
    rootLayout->addWidget(title);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto *content = new QWidget(scrollArea);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 8, 0);
    contentLayout->setSpacing(10);

    m_themeCombo = createOptions({tr("Dark"), tr("Light")});
    m_themeCombo->setCurrentIndex(m_themeManager->mode() == ThemeMode::Dark ? 0 : 1);
    contentLayout->addWidget(createSettingCard(
        tr("Appearance"), tr("Switch the application color theme immediately."), m_themeCombo));
    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, [this](const int index) {
        m_themeManager->setMode(index == 0 ? ThemeMode::Dark : ThemeMode::Light);
    });
    connect(m_themeManager, &ThemeManager::themeChanged, this, [this](const ThemeMode mode) {
        m_themeCombo->blockSignals(true);
        m_themeCombo->setCurrentIndex(mode == ThemeMode::Dark ? 0 : 1);
        m_themeCombo->blockSignals(false);
    });

    auto *msaa = createOptions({tr("Off"), tr("2x"), tr("4x"), tr("8x")});
    contentLayout->addWidget(createSettingCard(
        tr("MSAA"), tr("Anti-aliasing level for future plots and 3D views."), msaa));

    auto *fps = createOptions({tr("30 FPS"), tr("60 FPS"), tr("120 FPS")});
    fps->setCurrentIndex(1);
    contentLayout->addWidget(createSettingCard(
        tr("Plot refresh rate"), tr("Target refresh rate for simulated and live charts."), fps));

    auto *userCard = new QCheckBox(tr("Show"), this);
    userCard->setChecked(true);
    contentLayout->addWidget(createSettingCard(
        tr("User card"), tr("Show software information at the top of the navigation."), userCard));
    connect(userCard, &QCheckBox::toggled, this, &SettingsPage::userCardVisibilityChanged);

    auto *renderMode = createOptions({tr("Native"), tr("Raster"), tr("OpenGL")});
    contentLayout->addWidget(createSettingCard(
        tr("Window rendering"), tr("Rendering backend selection placeholder."), renderMode));

    auto *visualEffect = createOptions({tr("Standard"), tr("Mica"), tr("Acrylic")});
    contentLayout->addWidget(createSettingCard(
        tr("Window visual effect"), tr("Windows 11 backdrop integration placeholder."), visualEffect));

    auto *navigation = createOptions({tr("Expanded"), tr("Compact"), tr("Auto")});
    contentLayout->addWidget(createSettingCard(
        tr("Navigation mode"), tr("Choose expanded, compact, or width-aware navigation."), navigation));
    connect(navigation, &QComboBox::currentTextChanged,
            this, &SettingsPage::navigationModeChanged);

    auto *transition = createOptions({tr("None"), tr("Fade"), tr("Slide")});
    contentLayout->addWidget(createSettingCard(
        tr("Page transition"), tr("Animation selection reserved for a future version."), transition));

    const auto unavailable = [this](const QString &name, QComboBox *combo) {
        connect(combo, &QComboBox::activated, this, [this, name](int) {
            emit unavailableSettingRequested(name);
        });
    };
    unavailable(tr("MSAA"), msaa);
    unavailable(tr("Plot refresh rate"), fps);
    unavailable(tr("Window rendering"), renderMode);
    unavailable(tr("Window visual effect"), visualEffect);
    unavailable(tr("Page transition"), transition);

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
