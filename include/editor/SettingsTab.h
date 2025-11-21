#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace editor {

class SettingsTab : public QWidget {
    Q_OBJECT

  public:
    explicit SettingsTab(QWidget *parent = nullptr);

  private slots:
    void reloadFromDisk();
    void saveToDisk();

  private:
    void buildUi();
    void loadSettings();
    void setStatus(const QString &message);

    QString settingsPath_;
    QLineEdit *pathDisplay_ = nullptr;
    QSpinBox *bulkSpin_ = nullptr;
    QSpinBox *liquidSpin_ = nullptr;
    QSpinBox *containerSpin_ = nullptr;
    QSpinBox *ticksSpin_ = nullptr;
    QSpinBox *arrivalMinSpin_ = nullptr;
    QSpinBox *arrivalMaxSpin_ = nullptr;
    QSpinBox *dischargeMinSpin_ = nullptr;
    QSpinBox *dischargeMaxSpin_ = nullptr;
    QLabel *statusLabel_ = nullptr;
};

} // namespace editor
