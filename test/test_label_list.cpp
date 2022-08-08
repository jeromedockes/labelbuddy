#include "test_label_list.h"
#include "label_list.h"
#include "testing_utils.h"

namespace labelbuddy {
void TestLabelList::testLabelList() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  LabelListModel labelModel{};
  labelModel.setDatabase(dbName);
  LabelList labelList{};
  labelList.setModel(&labelModel);
  labelList.show();
  QTest::qWait(1000);
  auto lv = labelList.findChild<QListView*>();
  auto colBtn = labelList.findChildren<QPushButton*>()[2];
  auto scEdit = labelList.findChildren<QLineEdit*>()[1];
  QVERIFY(!colBtn->isEnabled());
  QVERIFY(!scEdit->isEnabled());
  lv->selectionModel()->select(labelModel.index(0, 0),
                               QItemSelectionModel::Select);
  QVERIFY(colBtn->isEnabled());
  QVERIFY(scEdit->isEnabled());
  QCOMPARE(scEdit->text(), QString("p"));
  scEdit->selectAll();
  QTest::keyClicks(scEdit, "5");
  QCOMPARE(scEdit->text(), QString("p"));
  QTest::keyClicks(scEdit, "x");
  QCOMPARE(scEdit->text(), QString("x"));
  QTest::keyClick(scEdit, Qt::Key_Enter);

  lv->selectionModel()->select(labelModel.index(0, 0),
                               QItemSelectionModel::Clear);
  lv->selectionModel()->select(labelModel.index(1, 0),
                               QItemSelectionModel::Select);
  QCOMPARE(scEdit->text(), QString(""));
  QTest::keyClicks(scEdit, "x");
  QCOMPARE(scEdit->text(), QString(""));
  QTest::keyClicks(scEdit, "y");
  QCOMPARE(scEdit->text(), QString("y"));
  QTest::keyClick(scEdit, Qt::Key_Enter);
  QCOMPARE(labelModel.rowCount(), 3);
  auto nlEdit = labelList.findChildren<QLineEdit*>()[0];
  QTest::keyClicks(nlEdit, "new label");
  QTest::keyClick(nlEdit, Qt::Key_Enter);
  QCOMPARE(labelModel.rowCount(), 4);
  QCOMPARE(scEdit->text(), QString(""));
  QTest::keyClicks(nlEdit, "label: Resumption of the session");
  QTest::keyClick(nlEdit, Qt::Key_Enter);
  QCOMPARE(labelModel.rowCount(), 4);
  QModelIndexList expected{labelModel.index(1, 0)};
  QCOMPARE(lv->selectionModel()->selectedIndexes(), expected);
  QCOMPARE(scEdit->text(), QString("y"));
  QTest::keyClicks(nlEdit, "    ");
  QTest::keyClick(nlEdit, Qt::Key_Enter);
  QCOMPARE(labelModel.rowCount(), 4);
  QCOMPARE(lv->selectionModel()->selectedIndexes(), expected);
}
} // namespace labelbuddy
