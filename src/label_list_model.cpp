#include <assert.h>

#include <QColor>
#include <QSqlDatabase>
#include <QSqlError>

#include "label_list_model.h"
#include "user_roles.h"
#include "utils.h"

namespace labelbuddy {

LabelListModel::LabelListModel(QObject* parent) : QSqlQueryModel(parent) {}

void LabelListModel::set_database(const QString& new_database_name) {
  assert(QSqlDatabase::contains(new_database_name));
  database_name = new_database_name;
  setQuery(select_query_text, QSqlDatabase::database(database_name));
}

QSqlQuery LabelListModel::get_query() const {
  return QSqlQuery(QSqlDatabase::database(database_name));
}

QVariant LabelListModel::data(const QModelIndex& index, int role) const {
  if (role == Qt::DisplayRole && index.column() == 0) {
    auto name = QSqlQueryModel::data(index, role).toString();
    auto key = data(index, Roles::ShortcutKeyRole).toString();
    if (key != QString()) {
      return QString("%0) %1").arg(key).arg(name);
    }
    return name;
  }
  if (role == Roles::RowIdRole) {
    if (index.column() != 0) {
      assert(!index.isValid());
      return QVariant{};
    }
    return QSqlQueryModel::data(index.sibling(index.row(), 1), Qt::DisplayRole);
  }
  if (role == Roles::ShortcutKeyRole) {
    auto label_id = data(index, Roles::RowIdRole).toInt();
    auto query = get_query();
    query.prepare("select shortcut_key from label where id = :labelid;");
    query.bindValue(":labelid", label_id);
    query.exec();
    query.next();
    return query.value(0).toString();
  }
  if (role == Qt::BackgroundRole) {
    auto label_id = data(index, Roles::RowIdRole).toInt();
    auto query = get_query();
    query.prepare("select color from label where id = :labelid;");
    query.bindValue(":labelid", label_id);
    query.exec();
    query.next();
    auto color = query.value(0).toString();
    assert(color != "");
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
    assert(false);
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
  setQuery(select_query_text, QSqlDatabase::database(database_name));
}

void LabelListModel::set_label_color(const QModelIndex& index,
                                     const QColor& color) {
  if (!color.isValid()) {
    return;
  }
  auto label_id = data(index, Roles::RowIdRole);
  if (label_id == QVariant()) {
    assert(false);
    return;
  }
  auto query = get_query();
  query.prepare("update label set color = :col where id = :labelid;");
  query.bindValue(":col", color.name());
  query.bindValue(":labelid", label_id.toInt());
  query.exec();
  assert(query.numRowsAffected() == 1);
  emit dataChanged(index, index, {Qt::BackgroundRole});
  emit labels_changed();
}

bool LabelListModel::is_valid_shortcut(const QString& shortcut,
                                       const QModelIndex& index) {
  auto label_id_variant = data(index, Roles::RowIdRole);
  int label_id = label_id_variant != QVariant() ? label_id_variant.toInt() : -1;
  return is_valid_shortcut(shortcut, label_id);
}

bool LabelListModel::is_valid_shortcut(const QString& shortcut, int label_id) {
  if (!re.match(shortcut).hasMatch()) {
    return false;
  }
  auto query = get_query();
  query.prepare("select id from label where shortcut_key = :shortcut "
                "and id != :labelid;");
  query.bindValue(":shortcut", shortcut);
  query.bindValue(":labelid", label_id);
  query.exec();
  if (query.next()) {
    return false;
  }
  return true;
}

void LabelListModel::set_label_shortcut(const QModelIndex& index,
                                        const QString& shortcut) {
  auto label_id = data(index, Roles::RowIdRole);
  if (label_id == QVariant()) {
    assert(false);
    return;
  }
  if (!re.match(shortcut).hasMatch()) {
    return;
  }
  auto query = get_query();
  query.prepare(
      "update label set shortcut_key = :shortcut where id = :labelid;");
  query.bindValue(":shortcut",
                  shortcut != "" ? shortcut : QVariant(QVariant::String));
  query.bindValue(":labelid", label_id.toInt());
  query.exec();
  // 19 = constraint violation (here, unique shortcut)
  // https://sqlite.org/rescode.html#constraint
  assert(query.numRowsAffected() == 1 ||
         query.lastError().nativeErrorCode() == "19");
  emit dataChanged(index, index, {Qt::DisplayRole});
  emit labels_changed();
}

int LabelListModel::add_label(const QString& name) {
  auto query = get_query();
  query.prepare("select id from label where name = :name;");
  query.bindValue(":name", name);
  query.exec();
  if (query.next()) {
    return query.value(0).toInt();
  }
  query.prepare("insert into label(name, color) values (:name, :color);");
  query.bindValue(":name", name);
  query.bindValue(":color", suggest_label_color());
  if (!query.exec()) {
    assert(false);
    return -1;
  }
  auto label_id = query.lastInsertId().toInt();
  refresh_current_query();
  emit labels_added();
  emit labels_changed();
  return label_id;
}
} // namespace labelbuddy
