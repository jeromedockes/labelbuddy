#include <assert.h>

#include <QSqlDatabase>
#include <QSqlError>

#include "doc_list_model.h"
#include "user_roles.h"

namespace labelbuddy {

DocListModel::DocListModel(QObject* parent) : QSqlQueryModel(parent) {}

QSqlQuery DocListModel::get_query() const {
  return QSqlQuery(QSqlDatabase::database(database_name));
}

void DocListModel::set_database(const QString& new_database_name) {
  assert(QSqlDatabase::contains(new_database_name));
  database_name = new_database_name;
  doc_filter = DocFilter::all;
  filter_label_id_ = -1;
  limit = 100;
  offset = 0;
  emit database_changed();
  refresh_current_query();
  emit labels_changed();
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
  auto default_flags = QSqlQueryModel::flags(index);
  if (index.column() == 0) {
    return default_flags;
  }
  return default_flags & (~Qt::ItemIsSelectable);
}

QList<QPair<QString, int>> DocListModel::get_label_names() const {
  auto query = get_query();
  query.exec("select name, id from label order by id;");
  QList<QPair<QString, int>> result{};
  while (query.next()) {
    result << QPair<QString, int>{query.value(0).toString(),
                                  query.value(1).toInt()};
  }
  return result;
}

void DocListModel::adjust_query(DocFilter new_doc_filter, int filter_label_id,
                                int new_limit, int new_offset) {
  limit = new_limit;
  offset = new_offset;
  doc_filter = new_doc_filter;
  filter_label_id_ = filter_label_id;
  result_set_outdated_ = false;

  auto query = get_query();
  switch (new_doc_filter) {
  case DocFilter::all:
    query.prepare("select replace(substr(coalesce(long_title, content), "
                  "1, 160), char(10), ' ') as head, id "
                  "from document order by id limit :lim offset :off;");
    break;
  case DocFilter::labelled:
    query.prepare("select replace(substr(coalesce(long_title, content), "
                  "1, 160), char(10), ' ') as head, id "
                  "from labelled_document order by id limit :lim offset :off;");
    break;
  case DocFilter::unlabelled:
    query.prepare(
        "select replace(substr(coalesce(long_title, content), 1, 160), "
        "char(10), ' ') as head, id "
        "from unlabelled_document order by id limit :lim offset :off;");
    break;
  case DocFilter::has_given_label:
    query.prepare(
        "select replace(substr(coalesce(long_title, content), 1, 160), "
        "char(10), ' ') as head, id "
        "from document where id in (select distinct doc_id from annotation "
        "where label_id = :labelid) order by id limit :lim offset :off;");
    query.bindValue(":labelid", filter_label_id);
    break;
  case DocFilter::not_has_given_label:
    query.prepare(
        "select replace(substr(coalesce(long_title, content), 1, 160), "
        "char(10), ' ') as head, id "
        "from document where id not in (select distinct doc_id from annotation "
        "where label_id = :labelid) order by id limit :lim offset :off;");
    query.bindValue(":labelid", filter_label_id);
    break;
  }

  query.bindValue(":lim", new_limit);
  query.bindValue(":off", new_offset);
  query.exec();
  assert(query.isActive());
  setQuery(query);
}

int DocListModel::total_n_docs(DocFilter doc_filter, int filter_label_id) {
  auto query = get_query();
  switch (doc_filter) {
  case DocFilter::labelled:
    return n_labelled_docs_;
  case DocFilter::unlabelled:
    return total_n_docs_no_filter() - n_labelled_docs_;
  case DocFilter::has_given_label:
    query.prepare("select count (*) from (select distinct doc_id from "
                  "annotation where label_id = :labelid)");
    query.bindValue(":labelid", filter_label_id);
    query.exec();
    query.next();
    return query.value(0).toInt();
  case DocFilter::not_has_given_label:
    query.prepare("select count (*) from (select distinct doc_id from "
                  "annotation where label_id = :labelid)");
    query.bindValue(":labelid", filter_label_id);
    query.exec();
    query.next();
    return total_n_docs_no_filter() - query.value(0).toInt();
  default:
    return total_n_docs_no_filter();
  }
}

int DocListModel::total_n_docs_no_filter() {
  auto query = get_query();
  query.exec("select count(*) from document;");
  query.next();
  return query.value(0).toInt();
}

void DocListModel::refresh_n_labelled_docs() {
  auto query = get_query();
  query.exec("select count (*) from "
             "(select distinct doc_id from annotation);");
  query.next();
  n_labelled_docs_ = query.value(0).toInt();
}

int DocListModel::delete_docs(const QModelIndexList& indices) {
  auto query = get_query();
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
  refresh_current_query();
  emit docs_deleted();
  return query.numRowsAffected();
}

int DocListModel::delete_all_docs(QProgressDialog* progress) {
  auto query = get_query();
  int n_deleted{};
  auto total = total_n_docs(DocListModel::DocFilter::all) + 1;
  if (progress != nullptr) {
    progress->setMaximum(total);
  }
  bool cancelled{};
  query.exec("begin transaction;");
  query.exec("delete from annotation;");
  do {
    if (progress != nullptr) {
      progress->setValue(n_deleted);
      if (progress->wasCanceled()) {
        cancelled = true;
        break;
      }
    }
    query.exec("delete from document limit 1000;");
    n_deleted += query.numRowsAffected();
    if (query.lastError().type() == QSqlError::StatementError) {
      // sqlite not compiled with SQLITE_ENABLE_UPDATE_DELETE_LIMIT
      // delete all in one go
      cancelled = !query.exec("delete from document;");
      n_deleted = query.numRowsAffected();
      break;
    }
    if (query.lastError().type() != QSqlError::NoError) {
      cancelled = true;
      break;
    }
  } while (query.numRowsAffected());
  if (cancelled) {
    n_deleted = 0;
    query.exec("rollback transaction;");
  } else {
    query.exec("commit transaction;");
  }
  if (progress != nullptr) {
    progress->setValue(total);
  }
  refresh_current_query();
  emit docs_deleted();
  return n_deleted;
}

void DocListModel::refresh_current_query() {
  refresh_n_labelled_docs();
  adjust_query(doc_filter, filter_label_id_, limit, offset);
}

void DocListModel::document_status_changed(DocumentStatus new_status) {
  if (doc_filter != DocFilter::all) {
    result_set_outdated_ = true;
  }
  if (new_status == DocumentStatus::Labelled) {
    ++n_labelled_docs_;
  } else {
    --n_labelled_docs_;
  }
}

void DocListModel::document_gained_label(int label_id, int doc_id) {
  (void)doc_id;
  if ((doc_filter == DocFilter::has_given_label ||
       doc_filter == DocFilter::not_has_given_label) &&
      filter_label_id_ == label_id) {
    result_set_outdated_ = true;
  }
}

void DocListModel::document_lost_label(int label_id, int doc_id) {
  (void)doc_id;
  if ((doc_filter == DocFilter::has_given_label ||
       doc_filter == DocFilter::not_has_given_label) &&
      filter_label_id_ == label_id) {
    result_set_outdated_ = true;
  }
}

void DocListModel::refresh_current_query_if_outdated() {
  if (result_set_outdated_) {
    refresh_current_query();
  }
}

} // namespace labelbuddy
