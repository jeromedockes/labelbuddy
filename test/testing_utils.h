#ifndef LABELBUDDY_TESTING_UTILS_H
#define LABELBUDDY_TESTING_UTILS_H

#include <QString>
#include <QTemporaryDir>

namespace labelbuddy {
QString prepare_db(QTemporaryDir&);
void add_annotations(const QString& db_name);
void add_many_docs(const QString& db_name);
QString example_doc();
QString long_doc();
} // namespace labelbuddy

#endif
