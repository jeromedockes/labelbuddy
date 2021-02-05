#include <QFile>
#include <QTextStream>

#include "utils.h"

namespace labelbuddy {

QString get_version() {
  QFile file(":/data/VERSION.txt");
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream in_stream(&file);
  return in_stream.readAll().trimmed();
}

} // namespace labelbuddy
