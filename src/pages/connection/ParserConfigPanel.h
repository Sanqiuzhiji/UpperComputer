#pragma once

#include <QFrame>

#include "models/ConnectionTypes.h"

class AppContext;
class QComboBox;
class QToolButton;

class ParserConfigPanel final : public QFrame
{
    Q_OBJECT

public:
    explicit ParserConfigPanel(AppContext *context, QWidget *parent = nullptr);

    [[nodiscard]] ParserMode parserMode() const;

signals:
    void parserModeChanged(ParserMode mode);
    void customProtocolChanged(const QString &protocolId);
    void requestProtocolLibrary();
    void helpRequested(const QString &message);

private:
    void updateCustomProtocolVisibility();
    [[nodiscard]] QString helpText() const;

    AppContext *m_context{};
    QToolButton *m_helpButton{};
    QComboBox *m_parserCombo{};
    QComboBox *m_protocolCombo{};
};
