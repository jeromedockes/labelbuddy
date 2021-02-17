#include <QSqlQuery>
#include <QSqlDatabase>

#include "testing_utils.h"
#include "database.h"

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

QString example_doc() {
  return u8R"(  this is some text
. and search for maçã1 or maçã2 "
   something else

maçã3

)";
}
} // namespace labelbuddy
