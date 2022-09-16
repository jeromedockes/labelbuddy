#ifndef LABELBUDDY_TEST_DATABASE_H
#define LABELBUDDY_TEST_DATABASE_H

#include <QtTest>

#include "database.h"

namespace labelbuddy {

class TestDatabase : public QObject {
  Q_OBJECT

private slots:
  void testOpenDatabase();
  void testLastOpenedDatabase();
  void testStoredDatabasePath();
  void testAppStateExtra();
  void testOpenDatabaseErrors();
  void testImportExportLabels();
  void testImportTxtLabels();
  void testImportLabelWithDuplicateShortcutKey();
  void testImportExportDocs();
  void testImportExportDocs_data();
  void testBatchImportExport();
  void testImportErrors_data();
  void testImportErrors();
  void testBadAnnotations();

  void cleanup();

private:
  QString createDocumentsFile(QTemporaryDir& tmpDir);
  void createDocumentsFileTxt(const QString& filePath, const QJsonArray& docs);
  void createDocumentsFileJson(const QString& filePath, const QJsonArray& docs);
  QString checkExportedDocs(DatabaseCatalog& catalog, QTemporaryDir& tmpDir);
  void checkExportedDocsJson(const QString& filePath, const QJsonArray& docs);
  void checkImportBack(DatabaseCatalog& catalog, const QString& exportFile);
  void checkDbLabels(QSqlQuery& query);
};
} // namespace labelbuddy
#endif
