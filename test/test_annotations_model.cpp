#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "annotations_model.h"
#include "database.h"
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
  model.delete_annotation(1);
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
  QCOMPARE(model.get_content(), QString(""));
}

void TestAnnotationsModel::test_surrogate_pairs() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  catalog.open_database(tmp_dir.filePath("db"));
  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  query.exec("insert into label(name) values ('label');");
  query.prepare("insert into document(content, content_md5) values (?, ?);");
  QString content("𝄞𝄞-𝄞");
  query.bindValue(0, content);
  auto hash =
      QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Md5);
  query.bindValue(1, hash);
  query.exec();
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 1);
  AnnotationsModel model{};
  model.set_database(catalog.get_current_database());

  QCOMPARE(model.code_point_idx_to_utf16_idx(0), 0);
  QCOMPARE(model.code_point_idx_to_utf16_idx(1), 2);
  QCOMPARE(model.code_point_idx_to_utf16_idx(2), 4);
  QCOMPARE(model.code_point_idx_to_utf16_idx(3), 5);
  QCOMPARE(model.code_point_idx_to_utf16_idx(4), 7);

  QCOMPARE(model.utf16_idx_to_code_point_idx(0), 0);
  QCOMPARE(model.utf16_idx_to_code_point_idx(1), 0);
  QCOMPARE(model.utf16_idx_to_code_point_idx(2), 1);
  QCOMPARE(model.utf16_idx_to_code_point_idx(3), 1);
  QCOMPARE(model.utf16_idx_to_code_point_idx(4), 2);
  QCOMPARE(model.utf16_idx_to_code_point_idx(5), 3);
  QCOMPARE(model.utf16_idx_to_code_point_idx(6), 3);

  model.add_annotation(1, 0, 2);
  query.exec("select start_char, end_char from annotation where rowid = 1;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);
  QCOMPARE(query.value(1).toInt(), 1);

  model.add_annotation(1, 4, 7);
  query.exec("select start_char, end_char from annotation where rowid = 2;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 2);
  QCOMPARE(query.value(1).toInt(), 4);

  content = "éé";
  hash = QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Md5);
  query.prepare("insert into document(content, content_md5) values (?, ?);");
  query.bindValue(0, content);
  query.bindValue(1, hash);
  query.exec();
  model.visit_doc(2);
  QCOMPARE(model.get_content(), content);

  QCOMPARE(model.code_point_idx_to_utf16_idx(0), 0);
  QCOMPARE(model.code_point_idx_to_utf16_idx(1), 1);
  QCOMPARE(model.code_point_idx_to_utf16_idx(2), 2);

  QCOMPARE(model.utf16_idx_to_code_point_idx(0), 0);
  QCOMPARE(model.utf16_idx_to_code_point_idx(1), 1);
  QCOMPARE(model.utf16_idx_to_code_point_idx(2), 2);
}

} // namespace labelbuddy
