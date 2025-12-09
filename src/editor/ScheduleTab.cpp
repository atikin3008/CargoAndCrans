#include "../include/editor/ScheduleTab.h"

#include "../include/editor/EditorFileUtils.h"

#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimeEdit>
#include <QVBoxLayout>
#include <algorithm>
#include <QStringList>
#include <optional>

namespace {
QString normalizeCargoType(const QString &value) {
    const QString lowered = value.trimmed().toLower();
    if (lowered == "bulk" || lowered == "liquid" || lowered == "container") {
        return lowered;
    }
    return QStringLiteral("bulk");
}
}

namespace editor {

ScheduleTab::ScheduleTab(QWidget *parent) : QWidget(parent) {
    schedulePath_ = locateProjectFile(QStringLiteral("schedule.json"));
    buildUi();
    loadSchedule();
}

void ScheduleTab::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);

    auto *pathLabel = new QLabel(tr("Файл: %1").arg(schedulePath_));
    mainLayout->addWidget(pathLabel);

    auto *contentLayout = new QHBoxLayout();
    table_ = new QTableWidget(0, ColumnCount, this);
    QStringList headers = {tr("ID"),       tr("Имя"),     tr("День"), tr("Время"),
                           tr("Груз"),     tr("Вес, т"), tr("Стоянка, д")};
    table_->setHorizontalHeaderLabels(headers);
    auto *header = table_->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);
    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setColumnHidden(ColumnId, true);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &ScheduleTab::handleSelectionChanged);
    connect(header, &QHeaderView::sectionClicked, this, &ScheduleTab::handleHeaderClicked);
    header->setSortIndicator(lastSortColumn_, lastSortOrder_);
    contentLayout->addWidget(table_, 2);

    auto *formWidget = new QWidget(this);
    auto *formLayout = new QFormLayout(formWidget);
    shipIdSpin_ = new QSpinBox();
    shipIdSpin_->setRange(1, 1000000);
    shipIdSpin_->setReadOnly(true);
    shipIdSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    nameEdit_ = new QLineEdit();
    arrivalDaySpin_ = new QSpinBox();
    arrivalDaySpin_->setRange(1, 366);
    arrivalTimeEdit_ = new QTimeEdit();
    arrivalTimeEdit_->setDisplayFormat("HH:mm");
    cargoCombo_ = new QComboBox();
    cargoCombo_->addItems({"bulk", "liquid", "container"});
    cargoCombo_->setEditable(true);
    cargoCombo_->setInsertPolicy(QComboBox::NoInsert);
    weightSpin_ = new QDoubleSpinBox();
    weightSpin_->setRange(0.0, 500000.0);
    weightSpin_->setDecimals(0);
    weightSpin_->setSingleStep(100.0);
    staySpin_ = new QSpinBox();
    staySpin_->setRange(1, 30);

    formLayout->addRow(tr("ID"), shipIdSpin_);
    formLayout->addRow(tr("Название"), nameEdit_);
    formLayout->addRow(tr("День прибытия"), arrivalDaySpin_);
    formLayout->addRow(tr("Время прибытия"), arrivalTimeEdit_);
    formLayout->addRow(tr("Тип груза"), cargoCombo_);
    formLayout->addRow(tr("Вес, тонн"), weightSpin_);
    formLayout->addRow(tr("Стоянка (дней)"), staySpin_);

    auto *buttonsLayout = new QVBoxLayout();
    auto *randomControls = new QHBoxLayout();
    auto *randomLabel = new QLabel(tr("Случайные корабли:"));
    randomCountSpin_ = new QSpinBox();
    randomCountSpin_->setRange(1, 50);
    randomCountSpin_->setValue(1);
    randomCountSpin_->setSuffix(tr(" шт."));
    auto *generateButton = new QPushButton(tr("Добавить"));
    randomControls->addWidget(randomLabel);
    randomControls->addWidget(randomCountSpin_);
    randomControls->addWidget(generateButton);
    randomControls->addStretch();

    auto *applyButton = new QPushButton(tr("Сохранить изменения"));
    auto *addButton = new QPushButton(tr("Добавить корабль"));
    auto *removeButton = new QPushButton(tr("Удалить корабль"));
    auto *reloadButton = new QPushButton(tr("Обновить"));
    auto *saveButton = new QPushButton(tr("Сохранить файл"));
    buttonsLayout->addLayout(randomControls);
    buttonsLayout->addWidget(applyButton);
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(removeButton);
    buttonsLayout->addWidget(reloadButton);
    buttonsLayout->addWidget(saveButton);
    buttonsLayout->addStretch();

    formLayout->addRow(buttonsLayout);

    contentLayout->addWidget(formWidget, 1);
    mainLayout->addLayout(contentLayout);

    statusLabel_ = new QLabel();
    mainLayout->addWidget(statusLabel_);

    connect(generateButton, &QPushButton::clicked, this, &ScheduleTab::handleGenerateRandom);
    connect(addButton, &QPushButton::clicked, this, &ScheduleTab::handleAddShip);
    connect(removeButton, &QPushButton::clicked, this, &ScheduleTab::handleRemoveShip);
    connect(applyButton, &QPushButton::clicked, this, &ScheduleTab::handleApplyChanges);
    connect(reloadButton, &QPushButton::clicked, this, &ScheduleTab::handleReload);
    connect(saveButton, &QPushButton::clicked, this, &ScheduleTab::handleSave);

    setFormEnabled(false);
}

bool ScheduleTab::loadSchedule() {
    QJsonDocument doc;
    QString error;
    if (!loadJsonDocument(schedulePath_, doc, error)) {
        QMessageBox::critical(this, tr("Ошибка"), error);
        return false;
    }
    if (!doc.isObject()) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Формат schedule.json поврежден"));
        return false;
    }

    const QJsonArray scheduleArray = doc.object().value("schedule").toArray();
    ships_.clear();
    ships_.reserve(scheduleArray.size());
    for (const QJsonValue &value : scheduleArray) {
        const QJsonObject obj = value.toObject();
        ShipRecord record;
        record.shipId = obj.value("ship_id").toInt();
        record.shipName = obj.value("ship_name").toString();
        record.arrivalDay = obj.value("arrival_date").toInt(1);
        const QString timeStr = obj.value("arrival_time").toString();
        QTime parsedTime = QTime::fromString(timeStr, "HH:mm");
        if (!parsedTime.isValid()) {
            parsedTime = QTime(0, 0);
        }
        record.arrivalTime = parsedTime;
        record.cargoType = normalizeCargoType(obj.value("cargo_type").toString());
        record.cargoWeight = obj.value("cargo_weight_tonnes").toDouble();
        record.plannedStay = obj.value("planned_stay_days").toInt(1);
        ships_.push_back(record);
    }
    reassignSequentialIds();
    sortRecords(lastSortColumn_, lastSortOrder_, std::nullopt);
    setStatus(tr("Загружено %1 записей").arg(static_cast<int>(ships_.size())));
    return true;
}

bool ScheduleTab::saveSchedule() {
    QJsonArray array;
    for (const auto &record : ships_) {
        QJsonObject obj;
        obj.insert("ship_id", record.shipId);
        obj.insert("ship_name", record.shipName);
        obj.insert("arrival_date", record.arrivalDay);
        obj.insert("arrival_time", record.arrivalTime.toString("HH:mm"));
        obj.insert("cargo_type", record.cargoType);
        obj.insert("cargo_weight_tonnes", record.cargoWeight);
        obj.insert("planned_stay_days", record.plannedStay);
        array.append(obj);
    }
    QJsonObject root;
    root.insert("schedule", array);
    QJsonDocument doc(root);
    QString error;
    if (!saveJsonDocument(schedulePath_, doc, error)) {
        QMessageBox::critical(this, tr("Ошибка"), error);
        return false;
    }
    setStatus(tr("Расписание сохранено"));
    return true;
}

void ScheduleTab::populateTable(const std::optional<int> &preferredSelection) {
    table_->setRowCount(static_cast<int>(ships_.size()));
    for (int row = 0; row < static_cast<int>(ships_.size()); ++row) {
        updateRow(row, ships_[row]);
    }

    if (ships_.empty()) {
        table_->clearSelection();
        setFormEnabled(false);
        return;
    }

    int rowToSelect = 0;
    if (preferredSelection.has_value()) {
        for (int row = 0; row < static_cast<int>(ships_.size()); ++row) {
            if (ships_[row].shipId == preferredSelection.value()) {
                rowToSelect = row;
                break;
            }
        }
    }
    table_->selectRow(rowToSelect);
    setFormEnabled(true);
}

void ScheduleTab::updateRow(int row, const ShipRecord &record) {
    const auto setItem = [&](int column, const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        table_->setItem(row, column, item);
    };
    setItem(0, QString::number(record.shipId));
    setItem(1, record.shipName);
    setItem(2, QString::number(record.arrivalDay));
    setItem(3, record.arrivalTime.toString("HH:mm"));
    setItem(4, record.cargoType);
    setItem(5, QString::number(record.cargoWeight));
    setItem(6, QString::number(record.plannedStay));
}

void ScheduleTab::fillForm(const ShipRecord &record) {
    shipIdSpin_->setValue(record.shipId);
    nameEdit_->setText(record.shipName);
    arrivalDaySpin_->setValue(record.arrivalDay);
    arrivalTimeEdit_->setTime(record.arrivalTime);
    const int cargoIndex = cargoCombo_->findText(record.cargoType, Qt::MatchFixedString);
    if (cargoIndex >= 0) {
        cargoCombo_->setCurrentIndex(cargoIndex);
    } else {
        cargoCombo_->setEditText(record.cargoType);
    }
    weightSpin_->setValue(record.cargoWeight);
    staySpin_->setValue(record.plannedStay);
}

void ScheduleTab::setFormEnabled(bool enabled) {
    shipIdSpin_->setEnabled(enabled);
    nameEdit_->setEnabled(enabled);
    arrivalDaySpin_->setEnabled(enabled);
    arrivalTimeEdit_->setEnabled(enabled);
    cargoCombo_->setEnabled(enabled);
    weightSpin_->setEnabled(enabled);
    staySpin_->setEnabled(enabled);
}

ShipRecord ScheduleTab::collectFormData() const {
    ShipRecord record;
    record.shipId = shipIdSpin_->value();
    record.shipName = nameEdit_->text();
    record.arrivalDay = arrivalDaySpin_->value();
    record.arrivalTime = arrivalTimeEdit_->time();
    record.cargoType = normalizeCargoType(cargoCombo_->currentText());
    record.cargoWeight = weightSpin_->value();
    record.plannedStay = staySpin_->value();
    return record;
}

bool ScheduleTab::hasSelection() const {
    return table_->currentRow() >= 0 && table_->currentRow() < table_->rowCount();
}

int ScheduleTab::currentRow() const {
    return table_->currentRow();
}

void ScheduleTab::setStatus(const QString &message) {
    statusLabel_->setText(message);
}

int ScheduleTab::nextShipId() const {
    return static_cast<int>(ships_.size()) + 1;
}

void ScheduleTab::reassignSequentialIds() {
    for (int i = 0; i < static_cast<int>(ships_.size()); ++i) {
        ships_[i].shipId = i + 1;
    }
}

int ScheduleTab::compareRecords(const ShipRecord &lhs, const ShipRecord &rhs, int column) const {
    auto compareInt = [](int a, int b) {
        if (a < b)
            return -1;
        if (a > b)
            return 1;
        return 0;
    };
    auto compareTime = [](const QTime &a, const QTime &b) {
        const int lhsValue = a.msecsSinceStartOfDay();
        const int rhsValue = b.msecsSinceStartOfDay();
        if (lhsValue < rhsValue)
            return -1;
        if (lhsValue > rhsValue)
            return 1;
        return 0;
    };
    auto compareString = [](const QString &a, const QString &b) { return QString::localeAwareCompare(a, b); };
    switch (column) {
    case ColumnId:
        return compareInt(lhs.shipId, rhs.shipId);
    case ColumnName:
        return compareString(lhs.shipName, rhs.shipName);
    case ColumnDay: {
        int cmp = compareInt(lhs.arrivalDay, rhs.arrivalDay);
        if (cmp != 0) {
            return cmp;
        }
        return compareTime(lhs.arrivalTime, rhs.arrivalTime);
    }
    case ColumnTime: {
        int cmp = compareTime(lhs.arrivalTime, rhs.arrivalTime);
        if (cmp != 0) {
            return cmp;
        }
        return compareInt(lhs.arrivalDay, rhs.arrivalDay);
    }
    case ColumnCargo:
        return compareString(lhs.cargoType, rhs.cargoType);
    case ColumnWeight:
        if (lhs.cargoWeight < rhs.cargoWeight)
            return -1;
        if (lhs.cargoWeight > rhs.cargoWeight)
            return 1;
        return 0;
    case ColumnStay:
        return compareInt(lhs.plannedStay, rhs.plannedStay);
    default:
        return 0;
    }
}

ShipRecord ScheduleTab::buildRandomShip() const {
    static const QStringList prefixes = {QStringLiteral("Atlantic"), QStringLiteral("Polar"), QStringLiteral("Northern"),
                                         QStringLiteral("Eastern"),  QStringLiteral("Aurora"), QStringLiteral("Ocean"),
                                         QStringLiteral("Arctic"),   QStringLiteral("Global"), QStringLiteral("Grand"),
                                         QStringLiteral("Silver")};
    static const QStringList suffixes = {QStringLiteral("Star"),   QStringLiteral("Spirit"), QStringLiteral("Trader"),
                                         QStringLiteral("Runner"), QStringLiteral("Skylark"), QStringLiteral("Voyager"),
                                         QStringLiteral("Express"), QStringLiteral("Carrier"), QStringLiteral("Legend"),
                                         QStringLiteral("Navigator")};
    static const QStringList cargoTypes = {QStringLiteral("bulk"), QStringLiteral("liquid"), QStringLiteral("container")};

    auto *rng = QRandomGenerator::global();
    ShipRecord record;
    record.shipId = nextShipId();
    const QString prefix = prefixes.at(rng->bounded(prefixes.size()));
    const QString suffix = suffixes.at(rng->bounded(suffixes.size()));
    record.shipName = prefix + QStringLiteral(" ") + suffix;
    record.arrivalDay = rng->bounded(1, 10);
    const int minutes = rng->bounded(24 * 60);
    record.arrivalTime = QTime(minutes / 60, minutes % 60);
    record.cargoType = cargoTypes.at(rng->bounded(cargoTypes.size()));
    auto randomInRange = [&](int minValue, int maxValue) {
        if (minValue == maxValue) {
            return minValue;
        }
        return rng->bounded(minValue, maxValue + 1);
    };
    if (record.cargoType == QStringLiteral("bulk")) {
        record.cargoWeight = randomInRange(10000, 60000);
    } else if (record.cargoType == QStringLiteral("liquid")) {
        record.cargoWeight = randomInRange(20000, 80000);
    } else {
        record.cargoWeight = randomInRange(5000, 40000);
    }
    record.plannedStay = randomInRange(1, 7);
    return record;
}

void ScheduleTab::sortRecords(int column, Qt::SortOrder order, std::optional<int> preferredSelection) {
    if (column < ColumnId || column >= ColumnCount) {
        column = ColumnDay;
    }
    lastSortColumn_ = column;
    lastSortOrder_ = order;
    if (!preferredSelection && hasSelection()) {
        const int row = currentRow();
        if (row >= 0 && row < static_cast<int>(ships_.size())) {
            preferredSelection = ships_[row].shipId;
        }
    }

    std::stable_sort(ships_.begin(), ships_.end(),
                     [&](const ShipRecord &lhs, const ShipRecord &rhs) {
                         int cmp = compareRecords(lhs, rhs, column);
                         if (cmp == 0) {
                             cmp = compareRecords(lhs, rhs, ColumnId);
                         }
                         if (cmp == 0) {
                             if (lhs.shipId < rhs.shipId) {
                                 cmp = -1;
                             } else if (lhs.shipId > rhs.shipId) {
                                 cmp = 1;
                             }
                         }
                         if (order == Qt::AscendingOrder) {
                             return cmp < 0;
                         }
                         return cmp > 0;
                     });
    populateTable(preferredSelection);
    if (table_ && table_->horizontalHeader()) {
        table_->horizontalHeader()->setSortIndicator(lastSortColumn_, lastSortOrder_);
    }
}

void ScheduleTab::handleSelectionChanged() {
    if (!hasSelection()) {
        setFormEnabled(false);
        return;
    }
    const int row = currentRow();
    setFormEnabled(true);
    fillForm(ships_.at(row));
}

void ScheduleTab::handleAddShip() {
    ShipRecord record;
    record.shipId = nextShipId();
    record.shipName = tr("Новый корабль");
    record.arrivalDay = 1;
    record.arrivalTime = QTime(0, 0);
    record.cargoType = QStringLiteral("bulk");
    record.cargoWeight = 1000.0;
    record.plannedStay = 1;
    ships_.push_back(record);
    sortRecords(lastSortColumn_, lastSortOrder_, record.shipId);
    setStatus(tr("Добавлен корабль с ID %1").arg(record.shipId));
}

void ScheduleTab::handleRemoveShip() {
    if (!hasSelection()) {
        QMessageBox::warning(this, tr("Удаление"), tr("Выберите корабль, который хотите удалить"));
        return;
    }
    const int row = currentRow();
    const int id = ships_.at(row).shipId;
    ships_.erase(ships_.begin() + row);
    reassignSequentialIds();
    std::optional<int> nextSelection;
    if (!ships_.empty()) {
        const int index = std::min(row, static_cast<int>(ships_.size()) - 1);
        nextSelection = ships_[index].shipId;
    }
    sortRecords(lastSortColumn_, lastSortOrder_, nextSelection);
    setStatus(tr("Удален корабль %1").arg(id));
}

void ScheduleTab::handleApplyChanges() {
    if (!hasSelection()) {
        QMessageBox::information(this, tr("Изменения"), tr("Выберите корабль для редактирования"));
        return;
    }
    const int row = currentRow();
    ShipRecord updated = collectFormData();
    ships_[row] = updated;
    sortRecords(lastSortColumn_, lastSortOrder_, updated.shipId);
    setStatus(tr("Корабль %1 обновлен").arg(updated.shipId));
}

void ScheduleTab::handleReload() {
    loadSchedule();
}

void ScheduleTab::handleSave() {
    saveSchedule();
}

void ScheduleTab::handleHeaderClicked(int logicalIndex) {
    if (logicalIndex < ColumnId || logicalIndex >= ColumnCount) {
        return;
    }
    Qt::SortOrder order = Qt::AscendingOrder;
    if (logicalIndex == lastSortColumn_) {
        order = (lastSortOrder_ == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    }
    sortRecords(logicalIndex, order);
}

void ScheduleTab::handleGenerateRandom() {
    if (!randomCountSpin_) {
        return;
    }
    int count = randomCountSpin_->value();
    if (count <= 0) {
        return;
    }
    std::optional<int> firstNewId;
    for (int i = 0; i < count; ++i) {
        ShipRecord record = buildRandomShip();
        ships_.push_back(record);
        if (!firstNewId.has_value()) {
            firstNewId = record.shipId;
        }
    }
    sortRecords(lastSortColumn_, lastSortOrder_, firstNewId);
    setStatus(tr("Добавлено %1 случайных кораблей").arg(count));
}

} // namespace editor
