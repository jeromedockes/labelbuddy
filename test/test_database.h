#ifndef LABELBUDDY_TEST_DATABASE_H
#define LABELBUDDY_TEST_DATABASE_H

#include <QtTest>

#include "database.h"

namespace labelbuddy {

class TestDatabase : public QObject {
  Q_OBJECT

private slots:
  void test_open_database();
  void test_last_opened_database();
  void test_stored_database_path();
  void test_app_state_extra();
  void test_open_database_errors();
  void test_import_export_labels();
  void test_import_label_with_duplicate_shortcut_key();
  void test_import_export_docs();
  void test_import_export_docs_data();
  void test_batch_import_export();

  void cleanup();

private:
  QString create_documents_file(QTemporaryDir& tmp_dir);
  void create_documents_file_txt(const QString& file_path,
                                 const QJsonArray& docs);
  void create_documents_file_xml(const QString& file_path,
                                 const QJsonArray& docs);
  void create_documents_file_json(const QString& file_path,
                                  const QJsonArray& docs);
  void create_documents_file_csv(const QString& file_path,
                                  const QJsonArray& docs);
  QString check_exported_docs(DatabaseCatalog& catalog, QTemporaryDir& tmp_dir);
  void check_exported_docs_xml(const QString& file_path,
                               const QJsonArray& docs);
  void check_exported_docs_json(const QString& file_path,
                                const QJsonArray& docs);
  void check_exported_docs_csv(const QString& file_path,
                                const QJsonArray& docs);
  void check_import_back(DatabaseCatalog& catalog, const QString& export_file);
  void check_db_labels(QSqlQuery& query);
};
} // namespace labelbuddy
#endif
