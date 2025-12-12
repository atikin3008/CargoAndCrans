#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace {
std::filesystem::path getExecutableDir(const char *argv0) {
    std::error_code ec;
    auto exePath = std::filesystem::canonical(std::filesystem::path(argv0), ec);
    if (ec) {
        exePath = std::filesystem::absolute(std::filesystem::path(argv0), ec);
    }
    return exePath.has_parent_path() ? exePath.parent_path() : std::filesystem::current_path();
}

std::optional<std::string> getEnvString(const char *name) {
    if (const char *value = std::getenv(name)) {
        if (*value != '\0') {
            return std::string(value);
        }
    }
    return std::nullopt;
}

std::filesystem::path locateBinary(const std::vector<std::string> &names, const std::filesystem::path &exeDir) {
    for (const auto &base : {std::filesystem::current_path(), exeDir, exeDir.parent_path()}) {
        for (const auto &name : names) {
            auto candidate = base / name;
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec)) {
                std::error_code canonicalEc;
                auto resolved = std::filesystem::canonical(candidate, canonicalEc);
                if (canonicalEc) {
                    return candidate;
                }
                return resolved;
            }
        }
    }
    return {};
}
} // namespace

class MenuWindow : public QWidget {
  public:
    explicit MenuWindow(const std::filesystem::path &exeDir, QWidget *parent = nullptr)
        : QWidget(parent), exeDir_(exeDir) {
        setWindowTitle("Cargo menu");
        setFixedSize(520, 280);

        simulationPath_ = getEnvString("SIMULATION_BIN").value_or(
            locateBinary({"Cargo_and_cranes", "Cargo_and_cranes.exe"}, exeDir_).string());
        editorPath_ =
            getEnvString("EDITOR_BIN").value_or(locateBinary({"editor", "Editor", "editor.exe"}, exeDir_).string());

        setupUi();
        updateUi();

        connect(&simulationProcess_, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
            if (state_ == State::SimRunning) {
                state_ = State::Main;
                setInfo("Симуляция завершилась.");
                updateUi();
            }
        });
    }

  private:
    enum class State { Main, SimRunning };

    void setupUi() {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(12);

        auto *title = new QLabel("Cargo & Cranes меню", this);
        title->setStyleSheet("font-size: 22px; font-weight: 600;");
        layout->addWidget(title);

        auto *buttonRow = new QVBoxLayout();

        simButton_ = new QPushButton(this);
        simButton_->setMinimumHeight(44);
        connect(simButton_, &QPushButton::clicked, this, [this]() {
            if (state_ == State::SimRunning) {
                restartSimulation();
            } else {
                startSimulation();
            }
        });
        buttonRow->addWidget(simButton_);

        stopButton_ = new QPushButton("Остановить симуляцию", this);
        stopButton_->setMinimumHeight(44);
        connect(stopButton_, &QPushButton::clicked, this, &MenuWindow::stopSimulation);
        buttonRow->addWidget(stopButton_);

        editorButton_ = new QPushButton("Editor", this);
        editorButton_->setMinimumHeight(44);
        connect(editorButton_, &QPushButton::clicked, this, &MenuWindow::launchEditor);
        buttonRow->addWidget(editorButton_);

        layout->addLayout(buttonRow);

        statusLabel_ = new QLabel(this);
        simPathLabel_ = new QLabel(this);
        infoLabel_ = new QLabel(this);
        infoLabel_->setStyleSheet("color: #4e7bdc;");

        layout->addWidget(statusLabel_);
        layout->addWidget(simPathLabel_);
        layout->addWidget(infoLabel_);
        layout->addStretch(1);
    }

    void updateUi() {
        const bool simAvailable = !simulationPath_.empty() && std::filesystem::exists(simulationPath_);
        const bool editorAvailable = !editorPath_.empty() && std::filesystem::exists(editorPath_);

        simButton_->setText(state_ == State::SimRunning ? "Перезапустить симуляцию" : "Запустить симуляцию");
        simButton_->setEnabled(simAvailable);

        stopButton_->setEnabled(state_ == State::SimRunning);

        editorButton_->setEnabled(editorAvailable);

        statusLabel_->setText(state_ == State::SimRunning ? "Симуляция: запущена" : "Симуляция: остановлена");
        simPathLabel_->setText(QString::fromStdString(
            "Путь симуляции: " + (simAvailable ? simulationPath_ : std::string("не найден"))));

        if (!infoMessage_.empty()) {
            infoLabel_->setText(QString::fromStdString(infoMessage_));
        } else {
            infoLabel_->clear();
        }
    }

    void startSimulation() {
        if (simulationPath_.empty() || !std::filesystem::exists(simulationPath_)) {
            setInfo("Бинарник симуляции не найден.");
            return;
        }

        if (simulationProcess_.state() != QProcess::NotRunning) {
            stopSimulation();
        }

        simulationProcess_.start(QString::fromStdString(simulationPath_));
        if (!simulationProcess_.waitForStarted(2000)) {
            setInfo("Не удалось запустить симуляцию.");
            return;
        }

        state_ = State::SimRunning;
        setInfo("Симуляция запущена.");
        updateUi();
    }

    void restartSimulation() {
        if (simulationProcess_.state() != QProcess::NotRunning) {
            stopSimulation();
        }
        startSimulation();
    }

    void stopSimulation() {
        if (simulationProcess_.state() != QProcess::NotRunning) {
            simulationProcess_.terminate();
            if (!simulationProcess_.waitForFinished(1200)) {
                simulationProcess_.kill();
                simulationProcess_.waitForFinished(1000);
            }
        }
        state_ = State::Main;
        setInfo("Симуляция остановлена.");
        updateUi();
    }

    void launchEditor() {
        if (editorPath_.empty() || !std::filesystem::exists(editorPath_)) {
            setInfo("Бинарник editor не найден.");
            return;
        }

        bool started = QProcess::startDetached(QString::fromStdString(editorPath_));
        if (!started) {
            setInfo("Не удалось запустить editor.");
        } else {
            setInfo("Editor запущен.");
        }
        updateUi();
    }

    void setInfo(const std::string &message) {
        infoMessage_ = message;
    }

    std::filesystem::path exeDir_;
    std::string simulationPath_;
    std::string editorPath_;
    State state_ = State::Main;
    QProcess simulationProcess_;

    QPushButton *simButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QPushButton *editorButton_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QLabel *simPathLabel_ = nullptr;
    QLabel *infoLabel_ = nullptr;
    std::string infoMessage_;
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    const auto exeDir = getExecutableDir(argv[0]);
    MenuWindow window(exeDir);
    window.show();
    return app.exec();
}
