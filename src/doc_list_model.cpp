#include <cassert>

#include <QSqlDatabase>
#include <QSqlError>

#include "doc_list_model.h"
#include "user_roles.h"

namespace labelbuddy {

DocListModel::DocListModel(QObject* parent) : QSqlQueryModel(parent) {}

QSqlQuery DocListModel::getQuery() const {
  return QSqlQuery(QSqlDatabase::database(databaseName_));
}

void DocListModel::setDatabase(const QString& newDatabaseName) {
  assert(QSqlDatabase::contains(newDatabaseName));
  databaseName_ = newDatabaseName;
  docFilter_ = DocFilter::all;
  filterLabelId_ = -1;
  limit_ = defaultNDocsLimit_;
  offset_ = 0;
  emit databaseChanged();
  refreshCurrentQuery();
  emit labelsChanged();
}

QVariant DocListModel::data(const QModelIndex& index, int role) const {
  if (role == Roles::RowIdRole) {
    if (index.column() != 0) {
      assert(false);
      return QVariant{};
    }
    return QSqlQueryModel::data(index.sibling(index.row(), 1), Qt::DisplayRole);
  }
  return QSqlQueryModel::data(index, role);
}

Qt::ItemFlags DocListModel::flags(const QModelIndex& index) const {
  auto defaultFlags = QSqlQueryModel::flags(index);
  if (index.column() == 0) {
    return defaultFlags;
  }
  return defaultFlags & (~Qt::ItemIsSelectable);
}

QList<QPair<QString, int>> DocListModel::getLabelNames() const {
  auto query = getQuery();
  query.exec("select name, id from sorted_label;");
  QList<QPair<QString, int>> result{};
  while (query.next()) {
    result << QPair<QString, int>{query.value(0).toString(),
                                  query.value(1).toInt()};
  }
  return result;
}

void DocListModel::adjustQuery(DocFilter newDocFilter, int filterLabelId,
                               int newLimit, int newOffset) {
  limit_ = newLimit;
  offset_ = newOffset;
  docFilter_ = newDocFilter;
  filterLabelId_ = filterLabelId;
  resultSetOutdated_ = false;

  auto query = getQuery();
  switch (newDocFilter) {
  case DocFilter::all:
    query.prepare("select replace(substr(coalesce(list_title, content), "
                  "1, 160), char(10), ' ') as head, id "
                  "from document order by id limit :lim offset :off;");
    break;
  case DocFilter::labelled:
    query.prepare("select replace(substr(coalesce(list_title, content), "
                  "1, 160), char(10), ' ') as head, id "
                  "from labelled_document order by id limit :lim offset :off;");
    break;
  case DocFilter::unlabelled:
    query.prepare(
        "select replace(substr(coalesce(list_title, content), 1, 160), "
        "char(10), ' ') as head, id "
        "from unlabelled_document order by id limit :lim offset :off;");
    break;
  case DocFilter::hasGivenLabel:
    query.prepare(
        "select replace(substr(coalesce(list_title, content), 1, 160), "
        "char(10), ' ') as head, id "
        "from document where id in (select distinct doc_id from annotation "
        "where label_id = :labelid) order by id limit :lim offset :off;");
    query.bindValue(":labelid", filterLabelId);
    break;
  case DocFilter::notHasGivenLabel:
    query.prepare(
        "select replace(substr(coalesce(list_title, content), 1, 160), "
        "char(10), ' ') as head, id "
        "from document where id not in (select distinct doc_id from annotation "
        "where label_id = :labelid) order by id limit :lim offset :off;");
    query.bindValue(":labelid", filterLabelId);
    break;
  }

  query.bindValue(":lim", newLimit);
  query.bindValue(":off", newOffset);
  query.exec();
  assert(query.isActive());
  setQuery(query);
}

int DocListModel::totalNDocs(DocFilter docFilter, int filterLabelId) {
  auto query = getQuery();
  switch (docFilter) {
  case DocFilter::labelled:
    return nLabelledDocs_;
  case DocFilter::unlabelled:
    return totalNDocsNoFilter() - nLabelledDocs_;
  case DocFilter::hasGivenLabel:
    query.prepare("select count (*) from (select distinct doc_id from "
                  "annotation where label_id = :labelid)");
    query.bindValue(":labelid", filterLabelId);
    query.exec();
    query.next();
    return query.value(0).toInt();
  case DocFilter::notHasGivenLabel:
    query.prepare("select count (*) from (select distinct doc_id from "
                  "annotation where label_id = :labelid)");
    query.bindValue(":labelid", filterLabelId);
    query.exec();
    query.next();
    return totalNDocsNoFilter() - query.value(0).toInt();
  default:
    return totalNDocsNoFilter();
  }
}

int DocListModel::totalNDocsNoFilter() {
  auto query = getQuery();
  query.exec("select count(*) from document;");
  query.next();
  return query.value(0).toInt();
}

void DocListModel::refreshNLabelledDocs() {
  auto query = getQuery();
  query.exec("select count (*) from "
             "(select distinct doc_id from annotation);");
  query.next();
  nLabelledDocs_ = query.value(0).toInt();
}

int DocListModel::deleteDocs(const QModelIndexList& indices) {
  auto query = getQuery();
  query.exec("begin transaction;");
  query.prepare("delete from document where id = ?");
  QVariantList ids;
  QVariant rowid;
  for (const QModelIndex& index : indices) {
    rowid = data(index, Roles::RowIdRole);
    if (rowid != QVariant()) {
      ids << rowid;
    } else {
      assert(false);
    }
  }
  query.addBindValue(ids);
  query.execBatch();
  query.exec("commit transaction;");
  refreshCurrentQuery();
  emit docsDeleted();
  return query.numRowsAffected();
}

int DocListModel::deleteAllDocs(QProgressDialog* progress) {
  auto query = getQuery();
  int nDeleted{};
  auto total = totalNDocs(DocListModel::DocFilter::all) + 1;
  if (progress != nullptr) {
    progress->setMaximum(total);
  }
  bool cancelled{};
  query.exec("begin transaction;");
  query.exec("delete from annotation;");
  do {
    if (progress != nullptr) {
      progress->setValue(nDeleted);
      if (progress->wasCanceled()) {
        cancelled = true;
        break;
      }
    }
    query.exec("delete from document limit 1000;");
    nDeleted += query.numRowsAffected();
    if (query.lastError().type() == QSqlError::StatementError) {
      // sqlite not compiled with SQLITE_ENABLE_UPDATE_DELETE_LIMIT
      // delete all in one go
      cancelled = !query.exec("delete from document;");
      nDeleted = query.numRowsAffected();
      break;
    }
    if (query.lastError().type() != QSqlError::NoError) {
      cancelled = true;
      break;
    }
  } while (query.numRowsAffected());
  if (cancelled) {
    nDeleted = 0;
    query.exec("rollback transaction;");
  } else {
    query.exec("commit transaction;");
  }
  if (progress != nullptr) {
    progress->setValue(total);
  }
  refreshCurrentQuery();
  emit docsDeleted();
  return nDeleted;
}

void DocListModel::refreshCurrentQuery() {
  refreshNLabelledDocs();
  adjustQuery(docFilter_, filterLabelId_, limit_, offset_);
}

void DocListModel::documentStatusChanged(DocumentStatus newStatus) {
  if (docFilter_ != DocFilter::all) {
    resultSetOutdated_ = true;
  }
  if (newStatus == DocumentStatus::Labelled) {
    ++nLabelledDocs_;
  } else {
    --nLabelledDocs_;
  }
}

void DocListModel::documentGainedLabel(int labelId, int docId) {
  (void)docId;
  if ((docFilter_ == DocFilter::hasGivenLabel ||
       docFilter_ == DocFilter::notHasGivenLabel) &&
      filterLabelId_ == labelId) {
    resultSetOutdated_ = true;
  }
}

void DocListModel::documentLostLabel(int labelId, int docId) {
  (void)docId;
  if ((docFilter_ == DocFilter::hasGivenLabel ||
       docFilter_ == DocFilter::notHasGivenLabel) &&
      filterLabelId_ == labelId) {
    resultSetOutdated_ = true;
  }
}

void DocListModel::refreshCurrentQueryIfOutdated() {
  if (resultSetOutdated_) {
    refreshCurrentQuery();
  }
}

} // namespace labelbuddy
