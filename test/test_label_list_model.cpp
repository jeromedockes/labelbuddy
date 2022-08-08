#include <memory>

#include <QMimeData>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "database.h"
#include "test_label_list_model.h"
#include "testing_utils.h"
#include "user_roles.h"

namespace labelbuddy {

void TestLabelListModel::testDeleteLabels() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  LabelListModel model{};
  model.setDatabase(dbName);
  QCOMPARE(model.rowCount(), 3);
  auto index = model.index(1, 0);
  QCOMPARE(model.data(index, Roles::RowIdRole).toInt(), 2);
  model.deleteLabels(QList<QModelIndex>{index});
  QCOMPARE(model.data(model.index(1, 0), Roles::RowIdRole).toInt(), 3);
  QCOMPARE(model.totalNLabels(), 2);
  QCOMPARE(model.labelIdToModelIndex(3), model.index(1, 0));
}

void TestLabelListModel::testGetData_data() {
  QTest::addColumn<int>("row");
  QTest::addColumn<int>("col");
  QTest::addColumn<int>("role");
  QTest::addColumn<QVariant>("result");

  auto display = static_cast<int>(Qt::DisplayRole);
  QTest::newRow("0 0 display")
      << 0 << 0 << display << QVariant("p) label: Reinício da sessão");
  QTest::newRow("1 0 display")
      << 1 << 0 << display << QVariant("label: Resumption of the session");
  QTest::newRow("2 0 display")
      << 2 << 0 << display << QVariant("label: Επαvάληψη της συvσδoυ");

  QTest::newRow("0 1 display") << 0 << 1 << display << QVariant("1");
  QTest::newRow("1 1 display") << 1 << 1 << display << QVariant("2");
  QTest::newRow("0 1 display") << 2 << 1 << display << QVariant("3");

  auto labelName = static_cast<int>(Roles::LabelNameRole);
  QTest::newRow("0 0 label_name")
      << 0 << 0 << labelName << QVariant("label: Reinício da sessão");
  QTest::newRow("1 0 label_name")
      << 1 << 0 << labelName << QVariant("label: Resumption of the session");
  QTest::newRow("2 0 label_name")
      << 2 << 0 << labelName << QVariant("label: Επαvάληψη της συvσδoυ");

  auto rowid = static_cast<int>(Roles::RowIdRole);
  QTest::newRow("0 0 id") << 0 << 0 << rowid << QVariant("1");
  QTest::newRow("1 0 id") << 1 << 0 << rowid << QVariant("2");
  QTest::newRow("2 0 id") << 2 << 0 << rowid << QVariant("3");

  auto shortcut = static_cast<int>(Roles::ShortcutKeyRole);
  QTest::newRow("0 0 shortcut") << 0 << 0 << shortcut << QVariant("p");
  QTest::newRow("1 0 shortcut") << 1 << 0 << shortcut << QVariant("");
  QTest::newRow("2 0 shortcut") << 2 << 0 << shortcut << QVariant("");

  auto bg = static_cast<int>(Qt::BackgroundRole);
  QTest::newRow("0 0 color") << 0 << 0 << bg << QVariant(QColor("#aec7e8"));
  QTest::newRow("1 0 color") << 1 << 0 << bg << QVariant(QColor("#ffbb78"));
  QTest::newRow("2 0 color") << 2 << 0 << bg << QVariant(QColor("#98df8a"));
}

void TestLabelListModel::testGetData() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  LabelListModel model{};
  model.setDatabase(dbName);
  QFETCH(int, row);
  QFETCH(int, col);
  QFETCH(int, role);
  QFETCH(QVariant, result);
  QCOMPARE(model.data(model.index(row, col), role), result);
}

void TestLabelListModel::testSetColor() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  LabelListModel model{};
  model.setDatabase(dbName);
  QSqlQuery query(QSqlDatabase::database(dbName));
  query.exec("select color from label where id = 2;");
  query.next();
  QCOMPARE(query.value(0).toString(), "#ffbb78");
  auto idx = model.index(1, 0);
  model.setLabelColor(idx, "yellow");
  query.exec("select color from label where id = 2;");
  query.next();
  QCOMPARE(query.value(0).toString(), "#ffff00");
  model.setLabelColor(idx, "not_a_color!!!!!!");
  query.exec("select color from label where id = 2;");
  query.next();
  QCOMPARE(query.value(0).toString(), "#ffff00");
  model.setLabelColor(idx, "");
  query.exec("select color from label where id = 2;");
  query.next();
  QCOMPARE(query.value(0).toString(), "#ffff00");
}

void TestLabelListModel::testSetShortcut() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  LabelListModel model{};
  model.setDatabase(dbName);
  QSqlQuery query(QSqlDatabase::database(dbName));
  QCOMPARE(model.rowCount(), 3);
  query.exec("select shortcut_key from label where id = 1;");
  query.next();
  QCOMPARE(query.value(0).toString(), QString("p"));
  auto index = model.index(1, 0);
  model.setLabelShortcut(index, "z");
  query.exec("select shortcut_key from label where id = 2;");
  query.next();
  QCOMPARE(query.value(0).toString(), QString("z"));
  model.setLabelShortcut(model.index(1, 0), "");
  query.exec("select shortcut_key from label where id = 2;");
  query.next();
  QVERIFY(query.value(0).isNull());
  model.setLabelShortcut(model.index(1, 0), "/");
  query.exec("select shortcut_key from label where id = 2;");
  query.next();
  QVERIFY(query.value(0).isNull());
  QVERIFY(query.value(0).isNull());
  model.setLabelShortcut(model.index(1, 0), "p");
  query.exec("select shortcut_key from label where id = 2;");
  query.next();
  QVERIFY(query.value(0).isNull());
  model.setLabelShortcut(model.index(1, 0), "x");
  query.exec("select shortcut_key from label where id = 2;");
  query.next();
  QCOMPARE(query.value(0).toString(), QString("x"));
}

void TestLabelListModel::testIsValidShortcut() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  LabelListModel model{};
  model.setDatabase(dbName);
  auto idx0 = model.index(0, 0);
  auto idx1 = model.index(1, 0);
  QCOMPARE(model.isValidShortcut("p", idx0), true);
  QCOMPARE(model.isValidShortcut("p", idx1), false);
  QCOMPARE(model.isValidShortcut("a", idx0), true);
  QCOMPARE(model.isValidShortcut("a", idx1), true);
  QCOMPARE(model.isValidShortcut("2", idx0), false);
  model.setLabelShortcut(idx0, "a");
  QCOMPARE(model.isValidShortcut("p", idx1), true);
}

void TestLabelListModel::testAddLabel() {
  DatabaseCatalog catalog{};
  LabelListModel model{};
  model.setDatabase(catalog.getCurrentDatabase());
  model.addLabel("label 1");
  QList<QString> expected{"label 1"};
  QCOMPARE(getLabelNames(model), expected);
  model.addLabel("label 1");
  QCOMPARE(getLabelNames(model), expected);
  model.addLabel("label 2");
  expected << "label 2";
  QCOMPARE(getLabelNames(model), expected);
}

void TestLabelListModel::testMimeDrop() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  LabelListModel model{};
  model.setDatabase(dbName);
  QList<int> expected{1, 2, 3};
  QCOMPARE(getLabelIds(model), expected);

  QModelIndexList selected{model.index(0, 0), model.index(2, 0)};
  std::unique_ptr<QMimeData> encoded{};
  encoded.reset(model.mimeData(selected));
  model.dropMimeData(encoded.get(), Qt::MoveAction, -1, 0, QModelIndex());
  expected = {2, 1, 3};
  QCOMPARE(getLabelIds(model), expected);

  selected.clear();
  selected << model.index(2, 0);
  encoded.reset(model.mimeData(selected));
  model.dropMimeData(encoded.get(), Qt::MoveAction, 0, 0, QModelIndex());
  expected = {3, 2, 1};
  QCOMPARE(getLabelIds(model), expected);

  selected.clear();
  selected << model.index(1, 0);
  encoded.reset(model.mimeData(selected));
  // drop before itself
  model.dropMimeData(encoded.get(), Qt::MoveAction, 1, 0, QModelIndex());
  expected = {3, 2, 1};
  QCOMPARE(getLabelIds(model), expected);
  // drop after itself
  model.dropMimeData(encoded.get(), Qt::MoveAction, 2, 0, QModelIndex());
  QCOMPARE(getLabelIds(model), expected);
}

QList<QString> getLabelNames(const LabelListModel& model) {
  QList<QString> result{};
  for (int i = 0; i != model.rowCount(); ++i) {
    result << model.data(model.index(i, 0), Roles::LabelNameRole).toString();
  }
  return result;
}
} // namespace labelbuddy
