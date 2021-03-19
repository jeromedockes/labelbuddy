#include <QSqlDatabase>

#include "doc_list_model.h"
#include "user_roles.h"

namespace labelbuddy {

DocListModel::DocListModel(QObject* parent) : QSqlQueryModel(parent) {}

QSqlQuery DocListModel::get_query() const {
  return QSqlQuery(QSqlDatabase::database(database_name));
}

void DocListModel::set_database(const QString& new_database_name) {
  database_name = new_database_name;
  refresh_current_query();
}

QVariant DocListModel::data(const QModelIndex& index, int role) const {
  if (role == Roles::RowIdRole) {
    if (index.column() != 0) {
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

void DocListModel::adjust_query(DocFilter new_doc_filter, int new_limit,
                                int new_offset) {
  limit = new_limit;
  offset = new_offset;
  doc_filter = new_doc_filter;

  auto query = get_query();
  switch (new_doc_filter) {

  case DocFilter::all:
    query.prepare("select replace(substr(coalesce(long_title, title, content), "
                  "1, 160), char(10), ' ') as head, id "
                  "from document order by id limit :lim offset :off;");
    break;

  case DocFilter::labelled:
    query.prepare("select replace(substr(coalesce(long_title, title, content), "
                  "1, 160), char(10), ' ') as head, id "
                  "from labelled_document order by id limit :lim offset :off;");
    break;

  case DocFilter::unlabelled:
    query.prepare(
        "select replace(substr(coalesce(long_title, title, content), 1, 160), "
        "char(10), ' ') as head, id "
        "from unlabelled_document order by id limit :lim offset :off;");
    break;
  }

  query.bindValue(":lim", new_limit);
  query.bindValue(":off", new_offset);
  query.exec();
  setQuery(query);
}

int DocListModel::total_n_docs(DocFilter doc_filter) {
  auto query = get_query();
  switch (doc_filter) {
  case DocFilter::all:
    query.prepare("select count(*) from document;");
    break;
  case DocFilter::labelled:
    query.prepare("select count (*) from "
                  "(select distinct doc_id from annotation);");
    break;
  case DocFilter::unlabelled:
    query.prepare("select count (*) from unlabelled_document;");
    break;
  }
  query.exec();
  query.next();
  return query.value(0).toInt();
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
  query.exec("begin transaction;");
  query.exec("delete from annotation;");
  do {
    if (progress != nullptr) {
      progress->setValue(n_deleted);
      if (progress->wasCanceled()) {
        break;
      }
    }
    query.exec("delete from document limit 1000;");
    n_deleted += query.numRowsAffected();
  } while (query.numRowsAffected());
  query.exec("commit transaction;");
  if (progress != nullptr) {
    progress->setValue(total);
  }
  refresh_current_query();
  emit docs_deleted();
  return n_deleted;
}

void DocListModel::refresh_current_query() {
  adjust_query(doc_filter, limit, offset);
}

} // namespace labelbuddy
