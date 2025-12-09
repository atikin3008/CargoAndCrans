#pragma once

#include <QMainWindow>

class QTabWidget;

namespace editor {

class SettingsTab;
class ScheduleTab;

class EditorWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit EditorWindow(QWidget *parent = nullptr);

  private:
    QTabWidget *tabWidget_ = nullptr;
    SettingsTab *settingsTab_ = nullptr;
    ScheduleTab *scheduleTab_ = nullptr;
};

} // namespace editor
