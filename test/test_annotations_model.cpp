#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "annotations_model.h"
#include "test_annotations_model.h"
#include "testing_utils.h"

namespace labelbuddy {
void TestAnnotationsModel::test_add_and_delete_annotations() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  AnnotationsModel model{};
  model.set_database(db_name);
  int id{};
  id = model.add_annotation(2, 3, 5);
  QCOMPARE(id, 1);
  id = model.add_annotation(1, 7, 9);
  QCOMPARE(id, 2);
  id = model.add_annotation(10, 7, 9);
  QCOMPARE(id, -1);
  id = model.add_annotation(3, 10, 12);
  QCOMPARE(id, 3);
  model.delete_annotations(QList<int>{1});
  QSqlQuery query(QSqlDatabase::database(db_name));
  query.exec(
      "select label_id from annotation where doc_id = 1 order by label_id;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 1);
  auto labels_info = model.get_labels_info();
  QCOMPARE(labels_info[3].color, QString("#98df8a"));
  auto annotations = model.get_annotations_info();
  QCOMPARE(annotations[3].end_char, 12);
}

void TestAnnotationsModel::test_navigation() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  AnnotationsModel model{};
  model.set_database(db_name);
  model.visit_next();
  QCOMPARE(model.current_doc_position(), 1);
  model.add_annotation(2, 3, 5);
  model.visit_next_unlabelled();
  QCOMPARE(model.current_doc_position(), 2);
  QVERIFY(model.get_content().startsWith("document 2"));
  model.visit_next();
  model.visit_next_unlabelled();
  QCOMPARE(model.current_doc_position(), 4);
  for (int i = 0; i != 20; ++i) {
    model.visit_next_unlabelled();
  }
  QCOMPARE(model.current_doc_position(), 5);
  QCOMPARE(model.get_annotations_info().size(), 0);
  model.visit_prev_labelled();
  QCOMPARE(model.current_doc_position(), 1);
  QCOMPARE(model.get_annotations_info().size(), 1);
  model.visit_prev_labelled();
  model.visit_prev_labelled();
  QCOMPARE(model.current_doc_position(), 1);
  model.visit_prev();
  QCOMPARE(model.current_doc_position(), 0);
  model.visit_next_unlabelled();
  QCOMPARE(model.current_doc_position(), 2);
  model.visit_prev();
  QSqlQuery query(QSqlDatabase::database(db_name));
  query.exec("delete from document where id = 2;");
  query.next();
  model.check_current_doc();
  QCOMPARE(model.current_doc_position(), 0);
  query.exec("delete from document;");
  query.next();
  model.check_current_doc();
  model.add_annotation(2, 3, 5);
  QCOMPARE(model.get_content(), QString(""));
}

} // namespace labelbuddy
