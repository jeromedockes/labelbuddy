#include <QFile>
#include <QFileInfo>
#include <QModelIndex>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>

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
  if(file_info.exists()) {
    return QUrl(QString("file:%0").arg(file_info.filePath()));
  }
  return QUrl("https://github.com/jeromedockes/labelbuddy");
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

} // namespace labelbuddy
