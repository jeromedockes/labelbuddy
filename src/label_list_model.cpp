#include <QColor>
#include <QSqlDatabase>

#include "label_list_model.h"
#include "user_roles.h"

namespace labelbuddy {

LabelListModel::LabelListModel(QObject* parent) : QSqlQueryModel(parent) {}

void LabelListModel::set_database(const QString& new_database_name) {
  database_name = new_database_name;
  setQuery("select name, id from label order by id;",
           QSqlDatabase::database(database_name));
}

QSqlQuery LabelListModel::get_query() const {
  return QSqlQuery(QSqlDatabase::database(database_name));
}

QVariant LabelListModel::data(const QModelIndex& index, int role) const {
  if (role == Roles::RowIdRole) {
    if (index.column() != 0) {
      return QVariant{};
    }
    return QSqlQueryModel::data(index.sibling(index.row(), 1), Qt::DisplayRole);
  }
  if (role == Qt::BackgroundRole) {
    auto label_id = data(index, Roles::RowIdRole).toInt();
    auto query = get_query();
    query.prepare("select color from label where id = :labelid;");
    query.bindValue(":labelid", label_id);
    query.exec();
    query.next();
    auto color = query.value(0).toString();
    return QColor(color);
  }
  return QSqlQueryModel::data(index, role);
}

Qt::ItemFlags LabelListModel::flags(const QModelIndex& index) const {
  auto default_flags = QSqlQueryModel::flags(index);
  if (index.column() == 0) {
    return default_flags;
  }
  return default_flags & (~Qt::ItemIsSelectable);
}

QModelIndex LabelListModel::label_id_to_model_index(int label_id) const {
  auto start = index(0, 0);
  auto matches =
      match(start, Roles::RowIdRole, QVariant(label_id), 1, Qt::MatchExactly);
  if (matches.length() == 0) {
    return QModelIndex();
  }
  return matches[0].sibling(matches[0].row(), 0);
}

int LabelListModel::total_n_labels() const {
  auto query = get_query();
  query.exec("select count(*) from label;");
  query.next();
  return query.value(0).toInt();
}

int LabelListModel::delete_labels(const QModelIndexList& indices) {
  auto query = get_query();
  query.exec("begin transaction;");
  query.prepare("delete from label where id = ?");
  QVariantList ids;
  QVariant rowid;
  for (const QModelIndex& index : indices) {
    rowid = data(index, Roles::RowIdRole);
    if (rowid != QVariant()) {
      ids << rowid;
    }
  }
  query.addBindValue(ids);
  query.execBatch();
  query.exec("commit transaction;");
  refresh_current_query();
  emit labels_deleted();
  emit labels_changed();
  return query.numRowsAffected();
}

void LabelListModel::refresh_current_query() {
  setQuery("select name, id from label order by id;",
           QSqlDatabase::database(database_name));
}

void LabelListModel::set_label_color(const QModelIndex& index,
                                     const QString& color) {
  auto label_id = data(index, Roles::RowIdRole);
  if (label_id == QVariant()) {
    return;
  }
  auto query = get_query();
  query.prepare("update label set color = :col where id = :labelid;");
  query.bindValue(":col", color);
  query.bindValue(":labelid", label_id.toInt());
  query.exec();
  emit labels_changed();
}

} // namespace labelbuddy
