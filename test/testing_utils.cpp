#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "database.h"
#include "testing_utils.h"

namespace labelbuddy {

QString prepare_db(QTemporaryDir& tmp_dir) {

  auto file_path = tmp_dir.filePath("db.sqlite");
  DatabaseCatalog catalog{};
  catalog.open_database(file_path);
  catalog.import_documents(":test/data/test_documents.json");
  catalog.import_labels(":test/data/test_labels.json");
  return file_path;
}

void add_annotations(const QString& db_name) {
  QSqlQuery query(QSqlDatabase::database(db_name));
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (1, 1, 0, 1);");
}

void add_many_docs(const QString& db_name) {
  QSqlQuery query(QSqlDatabase::database(db_name));
  query.exec("begin transaction;");
  for (int i = 0; i != 360; ++i) {
    query.prepare("insert into document (content, content_md5, title) values "
                  "(:content, :md5, :title);");
    auto content = QString("content of document %0").arg(i);
    query.bindValue(":content", content);
    query.bindValue(":md5", QCryptographicHash::hash(content.toUtf8(),
                                                     QCryptographicHash::Md5));
    query.bindValue(":title", QString("title of document %0").arg(i));
    query.exec();
  }
  query.exec("commit;");
}

QString example_doc() {
  return u8R"(  this is some text
. and search for maçã1 or maçã2 "
   something else

maçã3

)";
}

QString long_doc() {
  QString doc{};
  QTextStream doc_stream(&doc);
  for (int i = 0; i != 300; ++i) {
    doc_stream << "Line " << i << "\n";
  }
  return doc;
}

} // namespace labelbuddy
