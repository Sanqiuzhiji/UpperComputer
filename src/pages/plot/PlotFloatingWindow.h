#pragma once

#include <QMainWindow>

class PlotFloatingWindow final : public QMainWindow
{
    Q_OBJECT

public:
    PlotFloatingWindow(
        QWidget *page,
        const QString &title,
        QWidget *parent = nullptr);
    [[nodiscard]] QWidget *page() const noexcept;

signals:
    void reattachRequested(QWidget *page, const QString &title);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QWidget *m_page{};
    bool m_reattached{};
};
