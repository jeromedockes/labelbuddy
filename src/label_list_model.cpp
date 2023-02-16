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

void LabelListModel::setDatabase(const QString& newDatabaseName) {
  assert(QSqlDatabase::contains(newDatabaseName));
  databaseName_ = newDatabaseName;
  setQuery(selectQueryText_, QSqlDatabase::database(databaseName_));
}

QSqlQuery LabelListModel::getQuery() const {
  return QSqlQuery(QSqlDatabase::database(databaseName_));
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
    auto labelId = data(index, Roles::RowIdRole).toInt();
    auto query = getQuery();
    query.prepare("select shortcut_key from label where id = :labelid;");
    query.bindValue(":labelid", labelId);
    query.exec();
    query.next();
    return query.value(0).toString();
  }
  if (role == Qt::BackgroundRole) {
    auto labelId = data(index, Roles::RowIdRole).toInt();
    auto query = getQuery();
    query.prepare("select color from label where id = :labelid;");
    query.bindValue(":labelid", labelId);
    query.exec();
    query.next();
    auto color = query.value(0).toString();
    assert(color != "");
    return QColor(color);
  }
  return QSqlQueryModel::data(index, role);
}

Qt::ItemFlags LabelListModel::flags(const QModelIndex& index) const {
  auto itemFlags = QSqlQueryModel::flags(index);
  if (index.isValid()) {
    itemFlags |= Qt::ItemIsDragEnabled;
  } else {
    itemFlags |= Qt::ItemIsDropEnabled;
  }
  if (index.column() != 0) {
    itemFlags &= (~Qt::ItemIsSelectable);
  }
  return itemFlags;
}

Qt::DropActions LabelListModel::supportedDropActions() const {
  return Qt::MoveAction;
}

QMimeData* LabelListModel::mimeData(const QModelIndexList& indexes) const {
  QMap<int, int> sortedLabels{};
  for (const auto& idx : indexes) {
    auto labelId = data(idx, Roles::RowIdRole).toInt();
    assert(labelId != -1);
    assert(labelId != 0);
    sortedLabels.insert(idx.row(), labelId);
  }
  std::unique_ptr<QMimeData> mimeData{new QMimeData()};
  QByteArray encodedData{};
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  for (auto it = sortedLabels.cbegin(); it != sortedLabels.cend(); ++it) {
    stream << it.value();
  }
  mimeData->setData("application/labelbuddy.label_id_list", encodedData);
  // DragAction takes ownership of the mime data
  return mimeData.release();
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
  auto encodedData = data->data("application/labelbuddy.label_id_list");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  int labelId{-1};
  QList<int> movedLabels{};
  while (!stream.atEnd()) {
    stream >> labelId;
    movedLabels << labelId;
  }
  std::list<int> allLabels{};
  getReorderedLabelsInto(movedLabels, row, allLabels);
  assert(allLabels.size() == static_cast<size_t>(rowCount()));
  assert(setCompare(allLabels, getLabelIds(*this)));
  updateLabelsOrder(allLabels);
  return true;
}

void LabelListModel::getReorderedLabelsInto(const QList<int>& movedLabels,
                                            int row,
                                            std::list<int>& allLabels) const {
  auto query = getQuery();
  query.exec("select id from sorted_label;");
  auto insertPos = allLabels.cend();
  while (query.next()) {
    auto it = allLabels.insert(allLabels.cend(), query.value(0).toInt());
    if (allLabels.size() == static_cast<size_t>(row + 1)) {
      insertPos = it;
    }
  }
  assert((insertPos == allLabels.cend()) == (row == -1 || row == rowCount()));
  auto it = allLabels.cbegin();
  while (it != allLabels.cend()) {
    if (movedLabels.contains(*it)) {
      if (insertPos == it) {
        ++insertPos;
      }
      it = allLabels.erase(it);
    } else {
      ++it;
    }
  }
  allLabels.insert(insertPos, movedLabels.constBegin(), movedLabels.constEnd());
}

void LabelListModel::updateLabelsOrder(const std::list<int>& labels) {
  auto query = getQuery();
  int newPos = 0;
  query.exec("begin transaction;");
  for (auto labelId : labels) {
    query.prepare("update label set display_order = :pos where id = :id;");
    query.bindValue(":pos", newPos);
    query.bindValue(":id", labelId);
    query.exec();
    ++newPos;
  }
  query.exec("end transaction;");
  refreshCurrentQuery();
  emit labelsOrderChanged();
}

QStringList LabelListModel::mimeTypes() const {
  return {"application/labelbuddy_label_row_and_id_list"};
}

QModelIndex LabelListModel::labelIdToModelIndex(int labelId) const {
  auto start = index(0, 0);
  auto matches =
      match(start, Roles::RowIdRole, QVariant(labelId), 1, Qt::MatchExactly);
  if (matches.length() == 0) {
    assert(false);
    return QModelIndex();
  }
  return matches[0].sibling(matches[0].row(), 0);
}

int LabelListModel::totalNLabels() const {
  auto query = getQuery();
  query.exec("select count(*) from label;");
  query.next();
  return query.value(0).toInt();
}

int LabelListModel::deleteLabels(const QModelIndexList& indices) {
  auto query = getQuery();
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
  refreshCurrentQuery();
  emit labelsDeleted();
  emit labelsChanged();
  return query.numRowsAffected();
}

void LabelListModel::refreshCurrentQuery() {
  setQuery(selectQueryText_, QSqlDatabase::database(databaseName_));
}

void LabelListModel::setLabelColor(const QModelIndex& index,
                                   const QColor& color) {
  if (!color.isValid()) {
    return;
  }
  auto labelId = data(index, Roles::RowIdRole);
  if (labelId == QVariant()) {
    assert(false);
    return;
  }
  auto query = getQuery();
  query.prepare("update label set color = :col where id = :labelid;");
  query.bindValue(":col", color.name());
  query.bindValue(":labelid", labelId.toInt());
  query.exec();
  assert(query.numRowsAffected() == 1);
  emit dataChanged(index, index, {Qt::BackgroundRole});
  emit labelsChanged();
}

bool LabelListModel::isValidShortcut(const QString& shortcut,
                                     const QModelIndex& index) const {
  auto labelIdVariant = data(index, Roles::RowIdRole);
  int labelId = labelIdVariant != QVariant() ? labelIdVariant.toInt() : -1;
  return isValidShortcut(shortcut, labelId);
}

bool LabelListModel::isValidShortcut(const QString& shortcut,
                                     int labelId) const {
  if (!re_.match(shortcut).hasMatch()) {
    return false;
  }
  auto query = getQuery();
  query.prepare("select id from label where shortcut_key = :shortcut "
                "and id != :labelid;");
  query.bindValue(":shortcut", shortcut);
  query.bindValue(":labelid", labelId);
  query.exec();
  return !query.next();
}

void LabelListModel::setLabelShortcut(const QModelIndex& index,
                                      const QString& shortcut) {
  auto labelId = data(index, Roles::RowIdRole);
  if (labelId == QVariant()) {
    assert(false);
    return;
  }
  if (!re_.match(shortcut).hasMatch()) {
    return;
  }
  auto query = getQuery();
  query.prepare(
      "update label set shortcut_key = :shortcut where id = :labelid;");
  query.bindValue(":shortcut",
                  shortcut != "" ? shortcut : QVariant(QVariant::String));
  query.bindValue(":labelid", labelId.toInt());
  query.exec();
  // 19 = constraint violation (here, unique shortcut)
  // https://sqlite.org/rescode.html#constraint
  assert(query.numRowsAffected() == 1 ||
         query.lastError().nativeErrorCode() == "19");
  emit dataChanged(index, index, {Qt::DisplayRole});
  emit labelsChanged();
}

int LabelListModel::addLabel(const QString& name) {
  auto query = getQuery();
  query.prepare("select id from label where name = :name;");
  query.bindValue(":name", name);
  query.exec();
  if (query.next()) {
    return query.value(0).toInt();
  }
  query.prepare("insert into label(name, color) values (:name, :color);");
  query.bindValue(":name", name);
  query.bindValue(":color", suggestLabelColor());
  if (!query.exec()) {
    assert(false);
    return -1;
  }
  auto labelId = query.lastInsertId().toInt();
  refreshCurrentQuery();
  emit labelsAdded();
  emit labelsChanged();
  return labelId;
}

bool LabelListModel::isValidRename(const QString& newName,
                                   const QModelIndex& index) const {
  if (newName.isEmpty()) {
    return false;
  }
  auto labelIdVariant = data(index, Roles::RowIdRole);
  int labelId = labelIdVariant != QVariant() ? labelIdVariant.toInt() : -1;
  auto query = getQuery();
  query.prepare("select id from label where name = :name and id != :labelid;");
  query.bindValue(":name", newName);
  query.bindValue(":labelid", labelId);
  query.exec();
  return !query.next();
}

void LabelListModel::renameLabel(const QModelIndex& index,
                                 const QString& newName) {
  auto labelId = data(index, Roles::RowIdRole);
  if (labelId == QVariant()) {
    assert(false);
    return;
  }
  auto currentName = data(index, Roles::LabelNameRole).toString();
  if (currentName == newName) {
    return;
  }
  auto query = getQuery();
  query.prepare("update label set name = :name where id = :labelid;");
  query.bindValue(":name", newName);
  query.bindValue(":labelid", labelId.toInt());
  query.exec();
  // 19 = constraint violation (here, unique name or check name != '')
  // https://sqlite.org/rescode.html#constraint
  assert(query.numRowsAffected() == 1 ||
         query.lastError().nativeErrorCode() == "19");
  refreshCurrentQuery();
  emit dataChanged(index, index, {Qt::DisplayRole, Roles::LabelNameRole});
  emit labelsChanged();
  emit labelRenamed(labelId.toInt(), newName);
}

QList<int> getLabelIds(const LabelListModel& model) {
  QList<int> result{};
  for (int i = 0; i != model.rowCount(); ++i) {
    result << model.data(model.index(i, 0), Roles::RowIdRole).toInt();
  }
  return result;
}
} // namespace labelbuddy
