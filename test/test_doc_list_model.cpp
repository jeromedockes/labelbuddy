#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "doc_list_model.h"
#include "test_doc_list_model.h"
#include "testing_utils.h"
#include "user_roles.h"

namespace labelbuddy {

void TestDocListModel::test_delete_docs() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  DocListModel model{};
  model.set_database(db_name);
  QCOMPARE(model.rowCount(), 6);
  QList<QModelIndex> indices{model.index(1, 0), model.index(2, 0)};
  model.delete_docs(indices);
  QCOMPARE(model.rowCount(), 4);
  QSqlQuery query(QSqlDatabase::database(db_name));
  query.exec("select id from document order by id;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 1);
  query.next();
  QCOMPARE(query.value(0).toInt(), 4);
}

void TestDocListModel::test_filters() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  add_annotations(db_name);
  DocListModel model{};
  model.set_database(db_name);
  model.adjust_query(DocListModel::DocFilter::labelled);
  QCOMPARE(model.rowCount(), 1);
  QCOMPARE(model.data(model.index(0, 0), Roles::RowIdRole).toInt(), 1);
  model.adjust_query(DocListModel::DocFilter::unlabelled);
  QCOMPARE(model.rowCount(), 5);
  QCOMPARE(model.data(model.index(0, 0), Roles::RowIdRole).toInt(), 2);
  QCOMPARE(model.data(model.index(1, 0), Roles::RowIdRole).toInt(), 3);
  model.adjust_query(DocListModel::DocFilter::all);
  QCOMPARE(model.rowCount(), 6);
  QCOMPARE(model.data(model.index(3, 0), Roles::RowIdRole).toInt(), 4);
}

} // namespace labelbuddy
