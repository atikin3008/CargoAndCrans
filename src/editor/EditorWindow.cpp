#include "../include/editor/EditorWindow.h"

#include "../include/editor/ScheduleTab.h"
#include "../include/editor/SettingsTab.h"

#include <QTabWidget>

namespace editor {

EditorWindow::EditorWindow(QWidget *parent) : QMainWindow(parent) {
    tabWidget_ = new QTabWidget(this);
    settingsTab_ = new SettingsTab(tabWidget_);
    scheduleTab_ = new ScheduleTab(tabWidget_);

    tabWidget_->addTab(settingsTab_, tr("settings.json"));
    tabWidget_->addTab(scheduleTab_, tr("schedule.json"));

    setCentralWidget(tabWidget_);
    setWindowTitle(tr("Редактор настроек"));
    resize(1100, 700);
}

} // namespace editor
