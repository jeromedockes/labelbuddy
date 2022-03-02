#include <cassert>
#include <list>
#include <memory>

#include <QColor>
#include <QMimeData>
#include <QSqlDatabase>
#include <QSqlError>

#include "label_list_model.h"
#include "user_roles.h"
#include "utils.h"

namespace labelbuddy {

LabelListModel::LabelListModel(QObject* parent) : QSqlQueryModel(parent) {}

void LabelListModel::set_database(const QString& new_database_name) {
  assert(QSqlDatabase::contains(new_database_name));
  database_name_ = new_database_name;
  setQuery(select_query_text_, QSqlDatabase::database(database_name_));
}

QSqlQuery LabelListModel::get_query() const {
  return QSqlQuery(QSqlDatabase::database(database_name_));
}

QVariant LabelListModel::data(const QModelIndex& index, int role) const {
  if (role == LabelNameRole && index.column() == 0) {
    return QSqlQueryModel::data(index, Qt::DisplayRole).toString();
  }
  if (role == Qt::DisplayRole && index.column() == 0) {
    auto name = data(index, LabelNameRole).toString();
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
  auto item_flags = QSqlQueryModel::flags(index);
  if (index.isValid()) {
    item_flags |= Qt::ItemIsDragEnabled;
  } else {
    item_flags |= Qt::ItemIsDropEnabled;
  }
  if (index.column() != 0) {
    item_flags &= (~Qt::ItemIsSelectable);
  }
  return item_flags;
}

Qt::DropActions LabelListModel::supportedDropActions() const {
  return Qt::MoveAction;
}

QMimeData* LabelListModel::mimeData(const QModelIndexList& indexes) const {
  QMap<int, int> sorted_labels{};
  for (const auto& idx : indexes) {
    auto label_id = data(idx, Roles::RowIdRole).toInt();
    assert(label_id != -1);
    assert(label_id != 0);
    sorted_labels.insert(idx.row(), label_id);
  }
  std::unique_ptr<QMimeData> mime_data{new QMimeData()};
  QByteArray encoded_data{};
  QDataStream stream(&encoded_data, QIODevice::WriteOnly);
  for (auto it = sorted_labels.cbegin(); it != sorted_labels.cend(); ++it) {
    stream << it.value();
  }
  mime_data->setData("application/labelbuddy.label_id_list", encoded_data);
  // DragAction takes ownership of the mime data
  return mime_data.release();
}

bool LabelListModel::canDropMimeData(const QMimeData* data,
                                     Qt::DropAction action, int row, int column,
                                     const QModelIndex& parent) const {
  (void)row;
  (void)parent;
  if (action != Qt::MoveAction && action != Qt::IgnoreAction) {
    return false;
  }
  if (!data->hasFormat("application/labelbuddy.label_id_list")) {
    return false;
  }
  if (column > 0) {
    return false;
  }
  return true;
}

bool LabelListModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                  int row, int column,
                                  const QModelIndex& parent) {
  (void)parent;
  if (action == Qt::IgnoreAction) {
    return true;
  }
  if (!canDropMimeData(data, action, row, column, parent)) {
    return false;
  }
  auto encoded_data = data->data("application/labelbuddy.label_id_list");
  QDataStream stream(&encoded_data, QIODevice::ReadOnly);
  int label_id{-1};
  QList<int> moved_labels{};
  while (!stream.atEnd()) {
    stream >> label_id;
    moved_labels << label_id;
  }
  std::list<int> all_labels{};
  get_reordered_labels_into(moved_labels, row, all_labels);
  assert(all_labels.size() == static_cast<size_t>(rowCount()));
  assert (set_compare(all_labels, get_label_ids(*this)));
  update_labels_order(all_labels);
  return true;
}

void LabelListModel::get_reordered_labels_into(
    const QList<int>& moved_labels, int row, std::list<int>& all_labels) const {
  auto query = get_query();
  query.exec("select id from sorted_label;");
  auto insert_pos = all_labels.cend();
  while (query.next()) {
    auto it = all_labels.insert(all_labels.cend(), query.value(0).toInt());
    if (all_labels.size() == static_cast<size_t>(row + 1)) {
      insert_pos = it;
    }
  }
  assert((insert_pos == all_labels.cend()) == (row == -1 || row == rowCount()));
  auto it = all_labels.cbegin();
  while (it != all_labels.cend()) {
    if (moved_labels.contains(*it)) {
      if (insert_pos == it) {
        ++insert_pos;
      }
      it = all_labels.erase(it);
    } else {
      ++it;
    }
  }
  all_labels.insert(insert_pos, moved_labels.constBegin(),
                    moved_labels.constEnd());
}

void LabelListModel::update_labels_order(const std::list<int>& labels) {
  auto query = get_query();
  int new_pos = 0;
  query.exec("begin transaction;");
  for (auto label_id : labels) {
    query.prepare("update label set display_order = :pos where id = :id;");
    query.bindValue(":pos", new_pos);
    query.bindValue(":id", label_id);
    query.exec();
    ++new_pos;
  }
  query.exec("end transaction;");
  refresh_current_query();
  emit labels_order_changed();
}

QStringList LabelListModel::mimeTypes() const {
  return {"application/labelbuddy_label_row_and_id_list"};
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
  setQuery(select_query_text_, QSqlDatabase::database(database_name_));
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
                                       const QModelIndex& index) const {
  auto label_id_variant = data(index, Roles::RowIdRole);
  int label_id = label_id_variant != QVariant() ? label_id_variant.toInt() : -1;
  return is_valid_shortcut(shortcut, label_id);
}

bool LabelListModel::is_valid_shortcut(const QString& shortcut,
                                       int label_id) const {
  if (!re_.match(shortcut).hasMatch()) {
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
  if (!re_.match(shortcut).hasMatch()) {
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

QList<int> get_label_ids(const LabelListModel& model) {
  QList<int> result{};
  for (int i = 0; i != model.rowCount(); ++i) {
    result << model.data(model.index(i, 0), Roles::RowIdRole).toInt();
  }
  return result;
}
} // namespace labelbuddy
