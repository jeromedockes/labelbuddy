#include "test_label_list.h"
#include "label_list.h"
#include "testing_utils.h"

namespace labelbuddy {
void TestLabelList::test_label_list() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  LabelListModel label_model{};
  label_model.set_database(db_name);
  LabelList label_list{};
  label_list.setModel(&label_model);
  label_list.show();
  QTest::qWait(1000);
  auto lv = label_list.findChild<QListView*>();
  auto col_btn = label_list.findChildren<QPushButton*>()[2];
  auto sc_edit = label_list.findChild<QLineEdit*>();
  QVERIFY(!col_btn->isEnabled());
  QVERIFY(!sc_edit->isEnabled());
  lv->selectionModel()->select(label_model.index(0, 0),
                               QItemSelectionModel::Select);
  QVERIFY(col_btn->isEnabled());
  QVERIFY(sc_edit->isEnabled());
  QCOMPARE(sc_edit->text(), QString("p"));
  sc_edit->selectAll();
  QTest::keyClicks(sc_edit, "5");
  QCOMPARE(sc_edit->text(), QString("p"));
  QTest::keyClicks(sc_edit, "x");
  QCOMPARE(sc_edit->text(), QString("x"));
  QTest::keyClick(sc_edit, Qt::Key_Enter);

  lv->selectionModel()->select(label_model.index(0, 0),
                               QItemSelectionModel::Clear);
  lv->selectionModel()->select(label_model.index(1, 0),
                               QItemSelectionModel::Select);
  QCOMPARE(sc_edit->text(), QString(""));
  QTest::keyClicks(sc_edit, "x");
  QCOMPARE(sc_edit->text(), QString(""));
  QTest::keyClicks(sc_edit, "y");
  QCOMPARE(sc_edit->text(), QString("y"));
}
} // namespace labelbuddy
