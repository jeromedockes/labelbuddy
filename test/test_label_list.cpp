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
  auto sc_edit = label_list.findChildren<QLineEdit*>()[1];
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
  QTest::keyClick(sc_edit, Qt::Key_Enter);

  QCOMPARE(label_model.rowCount(), 3);
  auto nl_edit = label_list.findChildren<QLineEdit*>()[0];
  QTest::keyClicks(nl_edit, "new label");
  QTest::keyClick(nl_edit, Qt::Key_Enter);
  QCOMPARE(label_model.rowCount(), 4);
  QCOMPARE(sc_edit->text(), QString(""));
  QTest::keyClicks(nl_edit, "label: Resumption of the session");
  QTest::keyClick(nl_edit, Qt::Key_Enter);
  QCOMPARE(label_model.rowCount(), 4);
  QModelIndexList expected{label_model.index(1, 0)};
  QCOMPARE(lv->selectionModel()->selectedIndexes(), expected);
  QCOMPARE(sc_edit->text(), QString("y"));
  QTest::keyClicks(nl_edit, "    ");
  QTest::keyClick(nl_edit, Qt::Key_Enter);
  QCOMPARE(label_model.rowCount(), 4);
  QCOMPARE(lv->selectionModel()->selectedIndexes(), expected);

}
} // namespace labelbuddy
