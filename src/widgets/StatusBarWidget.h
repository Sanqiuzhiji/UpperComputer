#pragma once

#include <QWidget>

class QLabel;

class StatusBarWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBarWidget(QWidget *parent = nullptr);

public slots:
    void setCurrentPage(const QString &page);
    void setTheme(const QString &theme);

private:
    QLabel *m_page{};
    QLabel *m_theme{};
};
