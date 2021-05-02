#include <QComboBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>

#include "database.h"
#include "doc_list.h"
#include "test_doc_list.h"
#include "testing_utils.h"

namespace labelbuddy {

void TestDocList::test_delete_all_docs() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  add_annotations(db_name);
  DocListModel doc_model{};
  doc_model.set_database(db_name);
  DocList doc_list{};
  doc_list.setModel(&doc_model);

  QCOMPARE(doc_model.total_n_docs(), 6);
  QTimer::singleShot(1000, this, [&]() {
    doc_list.findChild<QMessageBox*>()
        ->findChildren<QPushButton*>()[0]
        ->click();
  });
  QTimer::singleShot(2000, this, [&]() {
    doc_list.findChild<QMessageBox*>()
        ->findChildren<QPushButton*>()[0]
        ->click();
  });
  doc_list.findChild<DocListButtons*>()
      ->findChildren<QPushButton*>()[2]
      ->click();
  QCOMPARE(doc_model.total_n_docs(), 0);
}

void TestDocList::test_delete_selected_docs() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  add_annotations(db_name);
  DocListModel doc_model{};
  doc_model.set_database(db_name);
  DocList doc_list{};
  doc_list.setModel(&doc_model);

  QSignalSpy spy(&doc_list, SIGNAL(n_selected_docs_changed(int)));

  QCOMPARE(doc_model.total_n_docs(), 6);
  QCOMPARE(doc_list.n_selected_docs(), 0);

  auto lv = doc_list.findChild<QListView*>();
  lv->selectionModel()->select(doc_model.index(0, 0),
                               QItemSelectionModel::Select);
  lv->selectionModel()->select(doc_model.index(2, 0),
                               QItemSelectionModel::Select);
  QCOMPARE(spy.count(), 2);
  QVERIFY(spy[1].at(0).toInt() == 2);
  QCOMPARE(doc_list.n_selected_docs(), 2);
  QTimer::singleShot(1000, this, [&]() {
    doc_list.findChild<QMessageBox*>()
        ->findChildren<QPushButton*>()[0]
        ->click();
  });
  QTimer::singleShot(2000, this, [&]() {
    doc_list.findChild<QMessageBox*>()
        ->findChildren<QPushButton*>()[0]
        ->click();
  });
  doc_list.findChild<DocListButtons*>()
      ->findChildren<QPushButton*>()[1]
      ->click();
  QCOMPARE(doc_model.total_n_docs(), 4);
  QCOMPARE(doc_list.n_selected_docs(), 0);
  QCOMPARE(spy.count(), 3);
  QVERIFY(spy[2].at(0).toInt() == 0);

  lv->selectAll();
  QCOMPARE(doc_list.n_selected_docs(), 4);
  QCOMPARE(spy.count(), 4);
  QVERIFY(spy[3].at(0).toInt() == 4);

  QTimer::singleShot(1000, this, [&]() {
    doc_list.findChild<QMessageBox*>()
        ->findChildren<QPushButton*>()[0]
        ->click();
  });
  QTimer::singleShot(2000, this, [&]() {
    doc_list.findChild<QMessageBox*>()
        ->findChildren<QPushButton*>()[0]
        ->click();
  });
  doc_list.findChild<DocListButtons*>()
      ->findChildren<QPushButton*>()[1]
      ->click();
  QCOMPARE(doc_model.total_n_docs(), 0);
  QCOMPARE(doc_list.n_selected_docs(), 0);
  QCOMPARE(spy.count(), 5);
  QVERIFY(spy[4].at(0).toInt() == 0);
}

void TestDocList::test_visit_document() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  add_annotations(db_name);
  DocListModel doc_model{};
  doc_model.set_database(db_name);
  DocList doc_list{};
  doc_list.setModel(&doc_model);

  QSignalSpy spy(&doc_list, SIGNAL(visit_doc_requested(int)));

  doc_list.findChild<DocListButtons*>()
      ->findChildren<QPushButton*>()[3]
      ->click();

  QCOMPARE(spy.count(), 0);

  auto lv = doc_list.findChild<QListView*>();
  lv->selectionModel()->select(doc_model.index(0, 0),
                               QItemSelectionModel::Select);

  doc_list.findChild<DocListButtons*>()
      ->findChildren<QPushButton*>()[3]
      ->click();

  QCOMPARE(spy.count(), 1);
  QVERIFY(spy[0].at(0).toInt() == 1);
}

void TestDocList::test_navigation() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  add_annotations(db_name);
  add_many_docs(db_name);
  DocListModel doc_model{};
  doc_model.set_database(db_name);
  DocList doc_list{};
  doc_list.setModel(&doc_model);
  QCOMPARE(doc_model.total_n_docs(), 366);

  auto filter_box = doc_list.findChild<QComboBox*>();
  auto buttons = doc_list.findChildren<QPushButton*>();
  auto first = buttons[4];
  auto prev = buttons[5];
  auto next = buttons[6];
  auto last = buttons[7];
  QLabel* label = doc_list.findChildren<QLabel*>().back();

  QVERIFY(!first->isEnabled());
  QVERIFY(!prev->isEnabled());
  QVERIFY(next->isEnabled());
  QVERIFY(last->isEnabled());

  next->click();
  QCOMPARE(label->text(), QString("101 - 200 / 366"));

  next->click();
  QCOMPARE(label->text(), QString("201 - 300 / 366"));

  last->click();
  QCOMPARE(label->text(), QString("301 - 366 / 366"));
  QVERIFY(!next->isEnabled());
  QVERIFY(!last->isEnabled());

  prev->click();
  QCOMPARE(label->text(), QString("201 - 300 / 366"));

  first->click();
  QCOMPARE(label->text(), QString("1 - 100 / 366"));

  last->click();
  QCOMPARE(label->text(), QString("301 - 366 / 366"));

  filter_box->setCurrentIndex(1);
  filter_box->activated(1);
  QCOMPARE(label->text(), QString("1 / 1"));
  QVERIFY(!first->isEnabled());
  QVERIFY(!prev->isEnabled());
  QVERIFY(!next->isEnabled());
  QVERIFY(!last->isEnabled());

  filter_box->setCurrentIndex(2);
  filter_box->activated(2);
  QCOMPARE(label->text(), QString("1 - 100 / 365"));

  filter_box->setCurrentIndex(0);
  filter_box->activated(0);
  QCOMPARE(label->text(), QString("1 - 100 / 366"));
}

void TestDocList::test_filters() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  add_annotations(db_name);
  DocListModel doc_model{};
  doc_model.set_database(db_name);
  DocList doc_list{};
  doc_list.setModel(&doc_model);
  QCOMPARE(doc_model.rowCount(), 6);

  auto filter_box = doc_list.findChild<QComboBox*>();

  // separators count as items
  QCOMPARE(filter_box->count(), 11);

  filter_box->setCurrentIndex(1);
  filter_box->activated(1);
  QCOMPARE(doc_model.rowCount(), 1);
  doc_model.labels_changed();
  QCOMPARE(filter_box->currentIndex(), 1);
  QCOMPARE(doc_model.rowCount(), 1);

  filter_box->setCurrentIndex(2);
  filter_box->activated(2);
  QCOMPARE(doc_model.rowCount(), 5);
  doc_model.labels_changed();
  QCOMPARE(filter_box->currentIndex(), 2);
  QCOMPARE(doc_model.rowCount(), 5);

  // label 1
  filter_box->setCurrentIndex(4);
  filter_box->activated(4);
  auto label_id = filter_box->currentData().value<QPair<int, int>>().first;
  QCOMPARE(label_id, 1);
  QCOMPARE(doc_model.rowCount(), 1);

  filter_box->setCurrentIndex(5);
  filter_box->activated(5);
  QCOMPARE(filter_box->currentText(),
           QString("label: Resumption of the session"));
  QCOMPARE(doc_model.rowCount(), 0);

  // not label 1
  filter_box->setCurrentIndex(8);
  filter_box->activated(8);
  QCOMPARE(doc_model.rowCount(), 5);

  filter_box->setCurrentIndex(10);
  filter_box->activated(10);
  QCOMPARE(doc_model.rowCount(), 6);

  // not label 1
  filter_box->setCurrentIndex(8);
  filter_box->activated(8);
  QCOMPARE(doc_model.rowCount(), 5);
  QSqlQuery query(QSqlDatabase::database(db_name));
  query.exec("delete from label where id = 2;");
  doc_model.labels_changed();
  QCOMPARE(filter_box->currentIndex(), 7);
  QCOMPARE(doc_model.rowCount(), 5);
  query.exec("delete from annotation;");
  doc_model.document_lost_label(1, 1);
  doc_list.show();
  QCOMPARE(doc_model.rowCount(), 6);

  auto new_db = tmp_dir.filePath("db1");
  { DatabaseCatalog().open_database(new_db); }
  doc_model.set_database(new_db);
  // separators have been removed
  QCOMPARE(filter_box->count(), 3);
  QCOMPARE(filter_box->currentIndex(), 0);
}
} // namespace labelbuddy
