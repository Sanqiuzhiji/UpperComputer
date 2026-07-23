#include "ThemeManager.h"

#include <QApplication>
#include <QStyle>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    apply();
}

ThemeMode ThemeManager::mode() const noexcept
{
    return m_mode;
}

QString ThemeManager::modeName() const
{
    return m_mode == ThemeMode::Dark ? tr("Dark") : tr("Light");
}

void ThemeManager::setMode(const ThemeMode mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    apply();
    emit themeChanged(m_mode);
}

void ThemeManager::toggleMode()
{
    setMode(m_mode == ThemeMode::Dark ? ThemeMode::Light : ThemeMode::Dark);
}

void ThemeManager::apply() const
{
    qApp->setStyleSheet(styleSheet());
    qApp->style()->unpolish(qApp);
    qApp->style()->polish(qApp);
}

QString ThemeManager::styleSheet() const
{
    const bool dark = m_mode == ThemeMode::Dark;
    const QString bg = dark ? QStringLiteral("#181A1F") : QStringLiteral("#F3F4F6");
    const QString panel = dark ? QStringLiteral("#202228") : QStringLiteral("#FFFFFF");
    const QString card = dark ? QStringLiteral("#25282E") : QStringLiteral("#FFFFFF");
    const QString hover = dark ? QStringLiteral("#30343C") : QStringLiteral("#E8EBEF");
    const QString border = dark ? QStringLiteral("#3A3E46") : QStringLiteral("#D7DAE0");
    const QString text = dark ? QStringLiteral("#E6E8EB") : QStringLiteral("#202329");
    const QString muted = dark ? QStringLiteral("#9DA3AE") : QStringLiteral("#6E747E");

    return QStringLiteral(R"(
        * { font-family: "Segoe UI"; font-size: 10pt; color: %1; }
        QMainWindow, QWidget#windowRoot, QWidget#pageArea { background: %2; }
        QWidget#titleBar, QWidget#sideNavigation, QWidget#statusBar { background: %3; }
        QFrame[card="true"] { background: %4; border: 1px solid %5; border-radius: 8px; }
        QLabel[muted="true"] { color: %6; }
        QPushButton, QToolButton {
            background: transparent; border: 1px solid transparent;
            border-radius: 6px; padding: 6px 10px;
        }
        QPushButton:hover, QToolButton:hover { background: %7; }
        QPushButton:pressed, QToolButton:pressed { background: %5; }
        QPushButton[accent="true"] { background: #28A9E0; color: white; }
        QPushButton[accent="true"]:hover { background: #43B7E7; }
        QPushButton[nav="true"] { text-align: left; padding: 9px 10px; border-radius: 5px; }
        QPushButton[nav="true"]:checked {
            background: %7; color: #28A9E0;
            border-left: 3px solid #28A9E0;
        }
        QComboBox, QSpinBox {
            background: %4; border: 1px solid %5; border-radius: 5px;
            padding: 5px 8px; min-height: 22px;
        }
        QComboBox QAbstractItemView { background: %4; selection-background-color: #28A9E0; }
        QScrollArea { border: none; background: transparent; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QStackedWidget { background: %2; }
        QCheckBox { spacing: 8px; }
        QCheckBox::indicator { width: 34px; height: 18px; }
        QCheckBox::indicator:unchecked {
            background: %5; border: 1px solid %5; border-radius: 9px;
        }
        QCheckBox::indicator:checked {
            background: #28A9E0; border: 1px solid #28A9E0; border-radius: 9px;
        }
        QToolTip { background: %4; color: %1; border: 1px solid %5; padding: 4px; }
        QLabel#pageTitle { font-size: 20pt; font-weight: 700; }
        QLabel#heroTitle { font-size: 24pt; font-weight: 700; color: #28A9E0; }
        QLabel#sectionTitle { font-weight: 700; }
        QLabel#telemetryValue { color: #28A9E0; font-weight: 600; }
        QLineEdit, QPlainTextEdit, QTreeWidget, QTableWidget, QMdiArea {
            background: %4; border: 1px solid %5; border-radius: 5px;
            selection-background-color: #28A9E0;
        }
        QHeaderView::section { background: %3; border: none; border-bottom: 1px solid %5; padding: 6px; }
        QSplitter::handle { background: %5; }
        QSplitter::handle:horizontal { width: 2px; }
        QSplitter::handle:vertical { height: 2px; }
        QSlider::groove:horizontal { height: 4px; background: %5; border-radius: 2px; }
        QSlider::handle:horizontal {
            width: 14px; margin: -5px 0; background: #28A9E0; border-radius: 7px;
        }
        QFrame[channelRow="true"]:hover { background: %7; border-radius: 4px; }
    )").arg(text, bg, panel, card, border, muted, hover);
}
