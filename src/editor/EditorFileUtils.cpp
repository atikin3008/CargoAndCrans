#include "../include/editor/EditorFileUtils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QObject>

namespace editor {
namespace {
QString tryToLocateFromRoot(const QString &root, const QString &fileName) {
    QDir dir(root);
    for (int depth = 0; depth < 5; ++depth) {
        const QString candidate = dir.absoluteFilePath(fileName);
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return {};
}
} // namespace

QString locateProjectFile(const QString &fileName) {
    const QStringList roots = {QDir::currentPath(), QCoreApplication::applicationDirPath()};
    for (const QString &root : roots) {
        const QString located = tryToLocateFromRoot(root, fileName);
        if (!located.isEmpty()) {
            return located;
        }
    }
    return QDir(QDir::currentPath()).absoluteFilePath(fileName);
}

bool loadJsonDocument(const QString &path, QJsonDocument &doc, QString &errorString) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = QObject::tr("Не удалось открыть %1: %2").arg(path, file.errorString());
        return false;
    }
    const QByteArray data = file.readAll();
    QJsonParseError parseError;
    doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        errorString = QObject::tr("Ошибка чтения %1: %2").arg(path, parseError.errorString());
        return false;
    }
    return true;
}

bool saveJsonDocument(const QString &path, const QJsonDocument &doc, QString &errorString) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = QObject::tr("Не удалось сохранить %1: %2").arg(path, file.errorString());
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

} // namespace editor
