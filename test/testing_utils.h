#ifndef LABELBUDDY_TESTING_UTILS_H
#define LABELBUDDY_TESTING_UTILS_H

#include <QString>
#include <QTemporaryDir>

namespace labelbuddy {
QString prepareDb(QTemporaryDir&);
void addAnnotations(const QString& dbName);
void addManyDocs(const QString& dbName);
QString exampleDoc();
QString longDoc();
} // namespace labelbuddy

#endif
