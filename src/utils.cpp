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

QString get_version() {
  QFile file(":/data/VERSION.txt");
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream in_stream(&file);
  return in_stream.readAll().trimmed();
}

QString get_welcome_message() {
  QFile file(":/data/welcome_message.html");
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream in_stream(&file);
  return in_stream.readAll();
}

QUrl get_doc_url() {
  QFileInfo file_info("/usr/share/doc/labelbuddy/documentation.html");
  if (file_info.exists()) {
    return QUrl(QString("file:%0").arg(file_info.filePath()));
  }
  QDir app_dir(QCoreApplication::applicationDirPath());
  file_info = QFileInfo(app_dir.filePath("documentation.html"));
  if (file_info.exists()) {
    return QUrl(QString("file:%0").arg(file_info.filePath()));
  }
  return QUrl("https://jeromedockes.github.io/labelbuddy/documentation.html");
}

QModelIndexList::const_iterator
find_first_in_col_0(const QModelIndexList& indices) {
  for (auto index = indices.constBegin(); index != indices.constEnd();
       ++index) {
    if (index->column() == 0) {
      return index;
    }
  }
  return indices.constEnd();
}

void prepare_parser(QCommandLineParser& parser) {
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

QRegularExpression shortcut_key_pattern(bool accept_empty) {
  return QRegularExpression{accept_empty ? "^[a-z]?$" : "^[a-z]$"};
}

QString database_name_display(const QString& database_name, bool full_path,
                              bool temp_warning) {
  if (database_name == ":LABELBUDDY_TEMPORARY_DATABASE:") {
    if (temp_warning) {
      return "Temporary database (will disappear when labelbuddy exits)";
    }
    return "Temporary database";
  }
  if (database_name == ":memory:") {
    if (temp_warning) {
      return "In-memory database (will disappear when labelbuddy exits)";
    }
    return "In-memory database";
  }
  if (full_path) {
    return database_name;
  }
  return QFileInfo(database_name).fileName();
}

void scale_margin(QWidget& widget, Side side, float scale) {
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

const QString& suggest_label_color() {
  static int color_index{};
  return suggest_label_color(color_index++);
}

const QString& suggest_label_color(int color_index) {
  static const QList<QString> label_colors{"#aec7e8", "#ffbb78", "#98df8a",
                                           "#ff9896", "#c5b0d5", "#c49c94",
                                           "#f7b6d2", "#dbdb8d", "#9edae5"};
  return label_colors[std::max(0, color_index) % label_colors.length()];
}

int cast_progress_to_range(double current, double maximum, double range_max) {
  return static_cast<int>(floor((current / std::max(maximum, 1.)) * range_max));
}

QString parent_directory(const QString& file_path) {
  auto dir = QDir(file_path);
  dir.cdUp();
  return dir.absolutePath();
}

} // namespace labelbuddy
