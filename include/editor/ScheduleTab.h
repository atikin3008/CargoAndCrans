#pragma once

#include <QTime>
#include <QWidget>
#include <optional>
#include <vector>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTimeEdit;

namespace editor {

struct ShipRecord {
    int shipId = 0;
    QString shipName;
    int arrivalDay = 1;
    QTime arrivalTime;
    QString cargoType;
    double cargoWeight = 0.0;
    int plannedStay = 1;
};

class ScheduleTab : public QWidget {
    Q_OBJECT

  public:
    explicit ScheduleTab(QWidget *parent = nullptr);

  private slots:
    void handleSelectionChanged();
    void handleAddShip();
    void handleRemoveShip();
    void handleApplyChanges();
    void handleReload();
    void handleSave();
    void handleHeaderClicked(int logicalIndex);
    void handleGenerateRandom();

  private:
    void buildUi();
    void populateTable(const std::optional<int> &preferredSelection = std::nullopt);
    void updateRow(int row, const ShipRecord &record);
    void fillForm(const ShipRecord &record);
    void setFormEnabled(bool enabled);
    ShipRecord collectFormData() const;
    bool hasSelection() const;
    int currentRow() const;
    void setStatus(const QString &message);
    int nextShipId() const;
    void reassignSequentialIds();
    void sortRecords(int column, Qt::SortOrder order, std::optional<int> preferredSelection = std::nullopt);
    int compareRecords(const ShipRecord &lhs, const ShipRecord &rhs, int column) const;
    ShipRecord buildRandomShip() const;

    bool loadSchedule();
    bool saveSchedule();

    enum Columns {
        ColumnId = 0,
        ColumnName,
        ColumnDay,
        ColumnTime,
        ColumnCargo,
        ColumnWeight,
        ColumnStay,
        ColumnCount
    };

    QString schedulePath_;
    QTableWidget *table_ = nullptr;
    QSpinBox *shipIdSpin_ = nullptr;
    QLineEdit *nameEdit_ = nullptr;
    QSpinBox *arrivalDaySpin_ = nullptr;
    QTimeEdit *arrivalTimeEdit_ = nullptr;
    QComboBox *cargoCombo_ = nullptr;
    QDoubleSpinBox *weightSpin_ = nullptr;
    QSpinBox *staySpin_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QSpinBox *randomCountSpin_ = nullptr;

    std::vector<ShipRecord> ships_;
    int lastSortColumn_ = ColumnDay;
    Qt::SortOrder lastSortOrder_ = Qt::AscendingOrder;
};

} // namespace editor
