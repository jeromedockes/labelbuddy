#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "label_list_model.h"
#include "test_label_list_model.h"
#include "testing_utils.h"
#include "user_roles.h"

namespace labelbuddy {

void TestLabelListModel::test_delete_labels() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  LabelListModel model{};
  model.set_database(db_name);
  QCOMPARE(model.rowCount(), 3);
  auto index = model.index(1, 0);
  QCOMPARE(model.data(index, Roles::RowIdRole).toInt(), 2);
  model.delete_labels(QList<QModelIndex>{index});
  QCOMPARE(model.data(model.index(1, 0), Roles::RowIdRole).toInt(), 3);
  QCOMPARE(model.total_n_labels(), 2);
  QCOMPARE(model.label_id_to_model_index(3), model.index(1, 0));
}

void TestLabelListModel::test_set_shortcut() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  LabelListModel model{};
  model.set_database(db_name);
  QSqlQuery query(QSqlDatabase::database(db_name));
  QCOMPARE(model.rowCount(), 3);
  query.exec("select shortcut_key from label where id = 1;");
  query.next();
  QCOMPARE(query.value(0).toString(), QString("p"));
  auto index = model.index(1, 0);
  model.set_label_shortcut(index, "z");
  query.exec("select shortcut_key from label where id = 2;");
  query.next();
  QCOMPARE(query.value(0).toString(), QString("z"));
  model.set_label_shortcut(model.index(1, 0), "");
  query.exec("select shortcut_key from label where id = 2;");
  query.next();
  QVERIFY(query.value(0).isNull());
  model.set_label_shortcut(model.index(1, 0), "/");
  query.exec("select shortcut_key from label where id = 2;");
  query.next();
  QVERIFY(query.value(0).isNull());
  QVERIFY(query.value(0).isNull());
  model.set_label_shortcut(model.index(1, 0), "p");
  query.exec("select shortcut_key from label where id = 2;");
  query.next();
  QVERIFY(query.value(0).isNull());
  model.set_label_shortcut(model.index(1, 0), "x");
  query.exec("select shortcut_key from label where id = 2;");
  query.next();
  QCOMPARE(query.value(0).toString(), QString("x"));
}
} // namespace labelbuddy
