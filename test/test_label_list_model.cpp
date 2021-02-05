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

} // namespace labelbuddy
