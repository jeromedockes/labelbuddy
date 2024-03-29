#include "test_label_list.h"
#include "label_list.h"
#include "testing_utils.h"
#include "user_roles.h"

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
  auto renameEdit = labelList.findChildren<QLineEdit*>()[2];

  QVERIFY(!colBtn->isEnabled());
  QVERIFY(!scEdit->isEnabled());
  QVERIFY(!renameEdit->isEnabled());
  lv->selectionModel()->select(labelModel.index(0, 0),
                               QItemSelectionModel::Select);
  QVERIFY(colBtn->isEnabled());
  QVERIFY(scEdit->isEnabled());
  QCOMPARE(scEdit->text(), QString("p"));
  QVERIFY(renameEdit->isEnabled());
  QCOMPARE(renameEdit->text(), QString(""));

  scEdit->selectAll();
  QTest::keyClicks(scEdit, "$");
  QCOMPARE(scEdit->text(), QString("p"));

  scEdit->selectAll();
  QTest::keyClicks(scEdit, "P");
  QCOMPARE(scEdit->text(), QString("P"));

  scEdit->selectAll();
  QTest::keyClicks(scEdit, "8");
  QCOMPARE(scEdit->text(), QString("8"));

  scEdit->selectAll();
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

  QTest::keyClicks(renameEdit, "new label renamed    ");
  QTest::keyClick(renameEdit, Qt::Key_Enter);
  QCOMPARE(labelModel.rowCount(), 4);
  QCOMPARE(
      labelModel.data(labelModel.index(3, 0), Roles::LabelNameRole).toString(),
      "new label renamed");

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
