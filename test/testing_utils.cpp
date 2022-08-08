#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "database.h"
#include "testing_utils.h"

namespace labelbuddy {

QString prepareDb(QTemporaryDir& tmpDir) {

  auto filePath = tmpDir.filePath("db.sqlite");
  DatabaseCatalog catalog{};
  catalog.openDatabase(filePath);
  catalog.importDocuments(":test/data/test_documents.json");
  catalog.importLabels(":test/data/test_labels.json");
  return filePath;
}

void addAnnotations(const QString& dbName) {
  QSqlQuery query(QSqlDatabase::database(dbName));
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char, "
             "extra_data) values (1, 1, 0, 1, 'hello extra data');");
}

void addManyDocs(const QString& dbName) {
  QSqlQuery query(QSqlDatabase::database(dbName));
  query.exec("begin transaction;");
  for (int i = 0; i != 360; ++i) {
    query.prepare("insert into document (content, content_md5) "
                  "values (:content, :md5);");
    auto content = QString("content of document %0").arg(i);
    query.bindValue(":content", content);
    query.bindValue(":md5", QCryptographicHash::hash(content.toUtf8(),
                                                     QCryptographicHash::Md5));
    query.exec();
  }
  query.exec("commit;");
}

QString exampleDoc() {
  return u8R"(  this is some text
. and search for maçã1 or maçã2 "
   something else

maçã3

)";
}

QString longDoc() {
  QString doc{};
  QTextStream docStream(&doc);
  for (int i = 0; i != 300; ++i) {
    docStream << "Line " << i << "\n";
  }
  return doc;
}

} // namespace labelbuddy
