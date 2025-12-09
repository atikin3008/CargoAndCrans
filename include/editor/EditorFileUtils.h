#pragma once

#include <QString>

class QJsonDocument;

namespace editor {

QString locateProjectFile(const QString &fileName);
bool loadJsonDocument(const QString &path, QJsonDocument &doc, QString &errorString);
bool saveJsonDocument(const QString &path, const QJsonDocument &doc, QString &errorString);

} // namespace editor
