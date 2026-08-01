#pragma once

#include "models/AppTypes.h"

#include <QColor>
#include <QPalette>

namespace PlotTheme {

inline QPalette palette(const ThemeMode mode)
{
    const bool dark = mode == ThemeMode::Dark;
    const QColor window(
        dark ? QStringLiteral("#202228") : QStringLiteral("#E9EDF2"));
    const QColor base(
        dark ? QStringLiteral("#25282E") : QStringLiteral("#FFFFFF"));
    const QColor alternate(
        dark ? QStringLiteral("#202228") : QStringLiteral("#F7F8FA"));
    const QColor border(
        dark ? QStringLiteral("#3A3E46") : QStringLiteral("#D7DAE0"));
    const QColor text(
        dark ? QStringLiteral("#E6E8EB") : QStringLiteral("#202329"));
    const QColor muted(
        dark ? QStringLiteral("#9DA3AE") : QStringLiteral("#6E747E"));

    QPalette result;
    result.setColor(QPalette::Window, window);
    result.setColor(QPalette::WindowText, text);
    result.setColor(QPalette::Base, base);
    result.setColor(QPalette::AlternateBase, alternate);
    result.setColor(QPalette::Text, text);
    result.setColor(QPalette::Button, base);
    result.setColor(QPalette::ButtonText, text);
    result.setColor(QPalette::PlaceholderText, muted);
    result.setColor(QPalette::Mid, border);
    result.setColor(QPalette::Highlight, QColor(QStringLiteral("#28A9E0")));
    result.setColor(QPalette::HighlightedText, Qt::white);
    return result;
}

} // namespace PlotTheme
