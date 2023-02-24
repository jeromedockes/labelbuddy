#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "doc_list_model.h"
#include "test_doc_list_model.h"
#include "testing_utils.h"
#include "user_roles.h"

namespace labelbuddy {

void TestDocListModel::testDeleteDocs() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  DocListModel model{};
  model.setDatabase(dbName);
  QCOMPARE(model.rowCount(), 6);
  QList<QModelIndex> indices{model.index(1, 0), model.index(2, 0)};
  model.deleteDocs(indices);
  QCOMPARE(model.rowCount(), 4);
  QSqlQuery query(QSqlDatabase::database(dbName));
  query.exec("select id from document order by id;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 1);
  query.next();
  QCOMPARE(query.value(0).toInt(), 4);
}

void TestDocListModel::testFilters() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  addAnnotations(dbName);
  DocListModel model{};
  model.setDatabase(dbName);
  model.adjustQuery(DocListModel::DocFilter::labelled);
  QCOMPARE(model.rowCount(), 1);
  QCOMPARE(model.data(model.index(0, 0), Roles::RowIdRole).toInt(), 1);
  model.adjustQuery(DocListModel::DocFilter::labelled, -1, "sexta-feira");
  QCOMPARE(model.rowCount(), 1);
  model.adjustQuery(DocListModel::DocFilter::labelled, -1, "fcbec15c87e");
  QCOMPARE(model.rowCount(), 1);
  model.adjustQuery(DocListModel::DocFilter::all, -1, "Fcbec15c87e");
  QCOMPARE(model.rowCount(), 0);
  model.adjustQuery(DocListModel::DocFilter::all, -1, "que");
  QCOMPARE(model.rowCount(), 2);
  // does not match 'requested'
  model.adjustQuery(DocListModel::DocFilter::all, -1, "'que '");
  QCOMPARE(model.rowCount(), 1);
  model.adjustQuery(DocListModel::DocFilter::all, -1, "Europe");
  QCOMPARE(model.rowCount(), 3);
  model.adjustQuery(DocListModel::DocFilter::all, -1, "europe");
  QCOMPARE(model.rowCount(), 3);
  model.adjustQuery(DocListModel::DocFilter::all, -1, R"("europe")");
  QCOMPARE(model.rowCount(), 0);
  model.adjustQuery(DocListModel::DocFilter::all, -1, R"("title")");
  QCOMPARE(model.rowCount(), 6);
  model.adjustQuery(DocListModel::DocFilter::all, -1, "europE");
  QCOMPARE(model.rowCount(), 0);
  model.adjustQuery(DocListModel::DocFilter::all, -1, "not in doc");
  QCOMPARE(model.rowCount(), 0);
  model.adjustQuery(DocListModel::DocFilter::all, -1, "Επαvάληψη της συvσδoυ");
  QCOMPARE(model.rowCount(), 1);
  model.adjustQuery(DocListModel::DocFilter::all, -1, "Επαvάληψη της συvσδoυ +");
  QCOMPARE(model.rowCount(), 0);
  model.adjustQuery(DocListModel::DocFilter::unlabelled);
  QCOMPARE(model.rowCount(), 5);
  QCOMPARE(model.data(model.index(0, 0), Roles::RowIdRole).toInt(), 2);
  QCOMPARE(model.data(model.index(1, 0), Roles::RowIdRole).toInt(), 3);
  model.adjustQuery(DocListModel::DocFilter::all);
  QCOMPARE(model.rowCount(), 6);
  QCOMPARE(model.data(model.index(3, 0), Roles::RowIdRole).toInt(), 4);
}

void TestDocListModel::testUpdatingResults() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  addAnnotations(dbName);
  DocListModel model{};
  model.setDatabase(dbName);
  model.adjustQuery(DocListModel::DocFilter::labelled);
  QCOMPARE(model.rowCount(), 1);
  QCOMPARE(model.totalNDocs(DocListModel::DocFilter::labelled), 1);
  QSqlQuery query(QSqlDatabase::database(dbName));

  query.exec("delete from annotation;");
  model.documentStatusChanged(DocumentStatus::Unlabelled);
  QCOMPARE(model.totalNDocs(DocListModel::DocFilter::labelled), 0);
  QCOMPARE(model.rowCount(), 1);
  model.refreshCurrentQueryIfOutdated();
  QCOMPARE(model.rowCount(), 0);

  addAnnotations(dbName);
  model.documentStatusChanged(DocumentStatus::Labelled);
  QCOMPARE(model.totalNDocs(DocListModel::DocFilter::labelled), 1);
  QCOMPARE(model.rowCount(), 0);
  model.refreshCurrentQueryIfOutdated();
  QCOMPARE(model.rowCount(), 1);
}

} // namespace labelbuddy
