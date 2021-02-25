#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
  parser.addPositionalArgument("database", "Database to open.");
  parser.addOption(
      {"demo", "Open a temporary demo database with pre-loaded docs"});
  parser.addOption(
      {"import-labels", "Labels file to import in database.", "labels file"});
  parser.addOption({"import-docs",
                    "Docs & annotations file to import in database.",
                    "docs file"});
  parser.addOption(
      {"export-labels", "Labels file to export to.", "exported labels file"});
  parser.addOption({"export-annotations",
                    "Docs & annotations file to export to.",
                    "exported docs file"});
  parser.addOption({"labelled-only", "Export only labelled documents"});
  parser.addOption(
      {"include-text", "Include doc text with exported annotations"});
  parser.addOption(
      {"approver", "User or 'annotations approver' name", "name", ""});
  parser.addOption(
      {"vacuum", "Repack database into minimal amount of disk space."});
}

} // namespace labelbuddy
