#include <QMessageBox>
#include <QTemporaryDir>
#include <QTimer>

#include "doc_list.h"
#include "doc_list_model.h"
#include "label_list_model.h"
#include "test_dataset_menu.h"
#include "testing_utils.h"

namespace labelbuddy {

void TestDatasetMenu::test_dataset_menu() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  add_annotations(db_name);
  DocListModel doc_model{};
  doc_model.set_database(db_name);
  LabelListModel label_model{};
  label_model.set_database(db_name);
  DatasetMenu menu{};
  menu.set_doc_list_model(&doc_model);
  menu.set_label_list_model(&label_model);
  auto doc_list = menu.findChild<DocList*>();
  auto filter_box = doc_list->findChildren<QComboBox*>()[0];
  filter_box->setCurrentIndex(1);
  filter_box->activated(1);
  QCOMPARE(doc_model.rowCount(), 1);
  QCOMPARE(menu.n_selected_docs(), 0);
  doc_list->findChildren<QPushButton*>()[0]->click();
  QCOMPARE(menu.n_selected_docs(), 1);
  auto lbuttons = menu.findChild<LabelListButtons*>();
  lbuttons->findChildren<QPushButton*>()[0]->click();
  QTimer::singleShot(1000, this, [&]() {
    menu.findChild<QMessageBox*>()->findChildren<QPushButton*>()[0]->click();
  });
  QTimer::singleShot(2000, this, [&]() {
    menu.findChild<QMessageBox*>()->findChildren<QPushButton*>()[0]->click();
  });
  lbuttons->findChildren<QPushButton*>()[1]->click();
  QTest::qWait(1000);
  QCOMPARE(doc_model.rowCount(), 0);
}

} // namespace labelbuddy
