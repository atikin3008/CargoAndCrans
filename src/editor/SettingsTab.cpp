#include "../include/editor/SettingsTab.h"

#include "../include/editor/EditorFileUtils.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
constexpr int kDefaultMinValue = -1000000;
constexpr int kDefaultMaxValue = 1000000;
}

namespace editor {

SettingsTab::SettingsTab(QWidget *parent) : QWidget(parent) {
    settingsPath_ = locateProjectFile(QStringLiteral("settings.json"));
    buildUi();
    loadSettings();
}

void SettingsTab::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);

    auto *pathLayout = new QHBoxLayout();
    auto *pathLabel = new QLabel(tr("Файл:"));
    pathDisplay_ = new QLineEdit(settingsPath_);
    pathDisplay_->setReadOnly(true);
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(pathDisplay_);
    mainLayout->addLayout(pathLayout);

    auto *cranesBox = new QGroupBox(tr("Количество кранов"));
    auto *cranesLayout = new QFormLayout(cranesBox);
    bulkSpin_ = new QSpinBox();
    bulkSpin_->setRange(0, 21);
    liquidSpin_ = new QSpinBox();
    liquidSpin_->setRange(0, 21);
    containerSpin_ = new QSpinBox();
    containerSpin_->setRange(0, 21);
    cranesLayout->addRow(tr("Bulk"), bulkSpin_);
    cranesLayout->addRow(tr("Liquid"), liquidSpin_);
    cranesLayout->addRow(tr("Container"), containerSpin_);
    mainLayout->addWidget(cranesBox);

    auto *simulationBox = new QGroupBox(tr("Параметры симуляции"));
    auto *simulationLayout = new QFormLayout(simulationBox);
    ticksSpin_ = new QSpinBox();
    ticksSpin_->setRange(0, kDefaultMaxValue * 10);
    arrivalMinSpin_ = new QSpinBox();
    arrivalMinSpin_->setRange(kDefaultMinValue, kDefaultMaxValue);
    arrivalMaxSpin_ = new QSpinBox();
    arrivalMaxSpin_->setRange(kDefaultMinValue, kDefaultMaxValue);
    dischargeMinSpin_ = new QSpinBox();
    dischargeMinSpin_->setRange(kDefaultMinValue, kDefaultMaxValue);
    dischargeMaxSpin_ = new QSpinBox();
    dischargeMaxSpin_->setRange(kDefaultMinValue, kDefaultMaxValue);

    simulationLayout->addRow(tr("Ticks amount"), ticksSpin_);
    simulationLayout->addRow(tr("Min arrival deviation"), arrivalMinSpin_);
    simulationLayout->addRow(tr("Max arrival deviation"), arrivalMaxSpin_);
    simulationLayout->addRow(tr("Min discharge deviation"), dischargeMinSpin_);
    simulationLayout->addRow(tr("Max discharge deviation"), dischargeMaxSpin_);
    mainLayout->addWidget(simulationBox);

    auto *buttonsLayout = new QHBoxLayout();
    auto *reloadButton = new QPushButton(tr("Обновить"));
    auto *saveButton = new QPushButton(tr("Сохранить"));
    connect(reloadButton, &QPushButton::clicked, this, &SettingsTab::reloadFromDisk);
    connect(saveButton, &QPushButton::clicked, this, &SettingsTab::saveToDisk);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(reloadButton);
    buttonsLayout->addWidget(saveButton);
    mainLayout->addLayout(buttonsLayout);

    statusLabel_ = new QLabel();
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    mainLayout->addWidget(statusLabel_);
    mainLayout->addStretch();
}

void SettingsTab::reloadFromDisk() {
    loadSettings();
}

void SettingsTab::saveToDisk() {
    QJsonObject root;
    QJsonObject cranes;
    cranes.insert("BULK", bulkSpin_->value());
    cranes.insert("LIQUID", liquidSpin_->value());
    cranes.insert("CONTAINER", containerSpin_->value());
    root.insert("cranes_amount", cranes);
    root.insert("ticks_amount", ticksSpin_->value());
    root.insert("minimum_deviation_of_arrival", arrivalMinSpin_->value());
    root.insert("maximum_deviation_of_arrival", arrivalMaxSpin_->value());
    root.insert("minimum_discharge_deviation", dischargeMinSpin_->value());
    root.insert("maximum_discharge_deviation", dischargeMaxSpin_->value());

    QJsonDocument doc(root);
    QString error;
    if (!saveJsonDocument(settingsPath_, doc, error)) {
        QMessageBox::critical(this, tr("Ошибка"), error);
        return;
    }
    setStatus(tr("Настройки сохранены"));
}

void SettingsTab::loadSettings() {
    QJsonDocument doc;
    QString error;
    if (!loadJsonDocument(settingsPath_, doc, error)) {
        QMessageBox::critical(this, tr("Ошибка"), error);
        return;
    }
    if (!doc.isObject()) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Формат settings.json поврежден"));
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonObject cranes = root.value("cranes_amount").toObject();
    bulkSpin_->setValue(static_cast<int>(cranes.value("BULK").toInt(bulkSpin_->value())));
    liquidSpin_->setValue(static_cast<int>(cranes.value("LIQUID").toInt(liquidSpin_->value())));
    containerSpin_->setValue(static_cast<int>(cranes.value("CONTAINER").toInt(containerSpin_->value())));

    ticksSpin_->setValue(root.value("ticks_amount").toInt(ticksSpin_->value()));
    arrivalMinSpin_->setValue(root.value("minimum_deviation_of_arrival").toInt(arrivalMinSpin_->value()));
    arrivalMaxSpin_->setValue(root.value("maximum_deviation_of_arrival").toInt(arrivalMaxSpin_->value()));
    dischargeMinSpin_->setValue(root.value("minimum_discharge_deviation").toInt(dischargeMinSpin_->value()));
    dischargeMaxSpin_->setValue(root.value("maximum_discharge_deviation").toInt(dischargeMaxSpin_->value()));

    setStatus(tr("Настройки загружены из файла"));
}

void SettingsTab::setStatus(const QString &message) {
    statusLabel_->setText(message);
}

} // namespace editor
