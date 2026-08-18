#pragma once

#include <QWidget>

class AppContext;
class CescFirmwareUploader;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;

class CescToolPage final : public QWidget
{
    Q_OBJECT

public:
    explicit CescToolPage(AppContext *context, QWidget *parent = nullptr);

private slots:
    void chooseFirmware();
    void startUpload();

private:
    void updateActions();

    AppContext *m_context{};
    CescFirmwareUploader *m_uploader{};
    QLineEdit *m_filePath{};
    QLabel *m_fileInfo{};
    QLabel *m_status{};
    QProgressBar *m_progress{};
    QPushButton *m_chooseButton{};
    QPushButton *m_uploadButton{};
    QPushButton *m_cancelButton{};
};
