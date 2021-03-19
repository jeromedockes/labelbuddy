#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalSpy>
#include <QTimer>

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

  QCOMPARE(doc_model.total_n_docs(), 6);

  auto lv = doc_list.findChild<QListView*>();
  lv->selectionModel()->select(doc_model.index(0, 0),
                               QItemSelectionModel::Select);
  lv->selectionModel()->select(doc_model.index(2, 0),
                               QItemSelectionModel::Select);
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
  lv->selectAll();
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

  auto radio = doc_list.findChildren<QRadioButton*>();
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

  radio[1]->click();
  QCOMPARE(label->text(), QString("1 / 1"));
  QVERIFY(!first->isEnabled());
  QVERIFY(!prev->isEnabled());
  QVERIFY(!next->isEnabled());
  QVERIFY(!last->isEnabled());

  radio[2]->click();
  QCOMPARE(label->text(), QString("1 - 100 / 365"));

  radio[0]->click();
  QCOMPARE(label->text(), QString("1 - 100 / 366"));
}

} // namespace labelbuddy
