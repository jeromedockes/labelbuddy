#ifndef LABELBUDDY_TEST_DATABASE_H
#define LABELBUDDY_TEST_DATABASE_H

#include <QtTest>

#include "database.h"

namespace labelbuddy {

class TestDatabase : public QObject {
  Q_OBJECT

private slots:
  void test_open_database();
  void test_import_export_docs();
  void test_import_export_docs_data();

private:
  QString create_documents_file(QTemporaryDir& tmp_dir);
  void create_documents_file_txt(const QString& file_path,
                                 const QJsonArray& docs);
  void create_documents_file_xml(const QString& file_path,
                                 const QJsonArray& docs);
  void create_documents_file_json(const QString& file_path,
                                  const QJsonArray& docs);
  void check_exported_docs(DatabaseCatalog& catalog, QTemporaryDir& tmp_dir);
  void check_exported_docs_xml(const QString& file_path,
                               const QJsonArray& docs);
  void check_exported_docs_json(const QString& file_path,
                                const QJsonArray& docs);
};
} // namespace labelbuddy
#endif
