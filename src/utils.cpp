#include <cmath>
#include <cstdlib>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QLayout>
#include <QModelIndex>
#include <QTextStream>

#include "utils.h"

namespace labelbuddy {

QString getVersion() {
  QFile file(":/data/VERSION.txt");
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream inStream(&file);
  return inStream.readAll().trimmed();
}

QString getWelcomeMessage() {
  QFile file(":/data/welcome_message.html");
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream inStream(&file);
  return inStream.readAll();
}

QUrl getDocUrl(const QString& pageName) {
  QStringList searchDirs{"/usr/share/doc/labelbuddy",
                         QCoreApplication::applicationDirPath()};

  for (const auto& directory : searchDirs) {
    QFileInfo fileInfo{directory, pageName + ".html"};
    if (fileInfo.exists()) {
      return QUrl::fromLocalFile(fileInfo.filePath());
    }
  }

  return QUrl{QString{
      "https://jeromedockes.github.io/labelbuddy/labelbuddy/current/%1/"}
                  .arg(pageName)};
}

QModelIndexList::const_iterator
findFirstInCol0(const QModelIndexList& indices) {
  for (auto index = indices.constBegin(); index != indices.constEnd();
       ++index) {
    if (index->column() == 0) {
      return index;
    }
  }
  return indices.constEnd();
}

void prepareParser(QCommandLineParser& parser) {
  parser.setApplicationDescription("Annotate documents.");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument("database", "Database to open.", "[database]");
  parser.addOption(
      {"demo", "Open a temporary demo database with pre-loaded docs."});
  parser.addOption(
      {"import-labels", "Labels file to import in database.", "labels file"});
  parser.addOption({"import-docs",
                    "Docs & annotations file to import in database.",
                    "docs file"});
  parser.addOption(
      {"export-labels", "Labels file to export to.", "exported labels file"});
  parser.addOption({"export-docs", "Docs & annotations file to export to.",
                    "exported docs file"});
  parser.addOption({"labelled-only", "Export only labelled documents."});
  parser.addOption({"no-text", "Do not include doc text when exporting."});
  parser.addOption(
      {"no-annotations", "Do not include annotations when exporting."});
  parser.addOption(
      {"vacuum", "Repack database into minimal amount of disk space."});
}

QRegularExpression shortcutKeyPattern(bool acceptEmpty) {
  return QRegularExpression{acceptEmpty ? "^[0-9A-Za-z]?$" : "^[0-9A-Za-z]$"};
}

QString shortcutKeyPatternDescription() {
  return "The shortcut key can be single letter or digit. For example: 'A', "
         "'a', '6'.";
}

QString databaseNameDisplay(const QString& databaseName, bool fullPath,
                            bool tempWarning) {
  if (databaseName == ":LABELBUDDY_TEMPORARY_DATABASE:") {
    if (tempWarning) {
      return "Temporary database (will disappear when labelbuddy exits)";
    }
    return "Temporary database";
  }
  if (databaseName == ":memory:") {
    if (tempWarning) {
      return "In-memory database (will disappear when labelbuddy exits)";
    }
    return "In-memory database";
  }
  if (fullPath) {
    return databaseName;
  }
  return QFileInfo(databaseName).fileName();
}

void scaleMargin(QWidget& widget, Side side, float scale) {
  auto margins = widget.layout()->contentsMargins();
  switch (side) {
  case Side::Left:
    margins.setLeft(
        static_cast<int>(static_cast<float>(margins.left()) * scale));
    break;
  case Side::Right:
    margins.setRight(
        static_cast<int>(static_cast<float>(margins.right()) * scale));
    break;
  }
  widget.layout()->setContentsMargins(margins);
}

const QString& suggestLabelColor() {
  static int colorIndex{};
  return suggestLabelColor(colorIndex++);
}

const QString& suggestLabelColor(int colorIndex) {
  static const QList<QString> labelColors{"#aec7e8", "#ffbb78", "#98df8a",
                                          "#ff9896", "#c5b0d5", "#c49c94",
                                          "#f7b6d2", "#dbdb8d", "#9edae5"};
  return labelColors[std::max(0, colorIndex) % labelColors.length()];
}

int castProgressToRange(double current, double maximum, double rangeMax) {
  return static_cast<int>(floor((current / std::max(maximum, 1.)) * rangeMax));
}

QString parentDirectory(const QString& filePath) {
  auto dir = QDir(filePath);
  dir.cdUp();
  return dir.absolutePath();
}

} // namespace labelbuddy
