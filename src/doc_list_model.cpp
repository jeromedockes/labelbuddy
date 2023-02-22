#include <cassert>

#include <QRegExp>
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
  searchPattern_ = "";
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
const QString DocListModel::sqlSourceSelect_ =
    " select replace(substr(coalesce(list_title, content), 1, 160), "
    "char(10), ' ') as head, id ";

const QString DocListModel::sqlSourceLike_ =
    R"( (list_title like :pat escape '\'
or display_title like :pat escape '\'
or cast(metadata as text) like :pat escape '\'
or content like :pat escape '\') )";

const QString DocListModel::sqlSourceInstr_ =
    R"( ( instr(list_title, :pat)
or instr(display_title, :pat)
or instr(cast(metadata as text), :pat)
or instr(content, :pat) ) )";

const QString DocListModel::sqlSourceOrder_ =
    " order by id limit :lim offset :off ";

QString DocListModel::getQueryText(DocFilter docFilter, bool withOrder,
                                   bool fullTitle, bool useInstr) {
  auto select = fullTitle ? sqlSourceSelect_ : " select id ";
  auto compare = useInstr ? sqlSourceInstr_ : sqlSourceLike_;
  auto order = withOrder ? sqlSourceOrder_ : " ";
  switch (docFilter) {
  case DocFilter::all:
    return select + "from document where" + compare + order;
  case DocFilter::labelled:
    return select + "from labelled_document where" + compare + order;
  case DocFilter::unlabelled:
    return select + "from unlabelled_document where" + compare + order;
  case DocFilter::hasGivenLabel:
    return select + "from document where" + compare +
           "and ( id in (select distinct doc_id from annotation where "
           "label_id = :labelid) )" +
           order;
  case DocFilter::notHasGivenLabel:
    return select + "from document where" + compare +
           "and ( id not in (select distinct doc_id from annotation "
           "where label_id = :labelid) )" +
           order;
  default:
    assert(false);
    return "";
  }
}

void DocListModel::prepareQuery(QSqlQuery& query, DocFilter docFilter,
                                int filterLabelId, const QString& searchPattern,
                                int limit, int offset) {
  auto caseSensitive = shouldBeCaseSensitive(searchPattern);
  auto pattern = transformSearchPattern(searchPattern);
  if (!caseSensitive) {
    pattern = transformLikePattern(pattern);
  }
  auto queryText = getQueryText(docFilter, true, true, caseSensitive) + ";";
  query.prepare(queryText);
  if (queryText.contains(":labelid")) {
    query.bindValue(":labelid", filterLabelId);
  }
  query.bindValue(":lim", limit);
  query.bindValue(":off", offset);
  query.bindValue(":pat", pattern);
}

void DocListModel::prepareCountQuery(QSqlQuery& query, DocFilter docFilter,
                                     int filterLabelId,
                                     const QString& searchPattern) {

  auto caseSensitive = shouldBeCaseSensitive(searchPattern);
  auto pattern = transformSearchPattern(searchPattern);
  if (!caseSensitive) {
    pattern = transformLikePattern(pattern);
  }
  auto queryText = "select count (*) from ( " +
                   getQueryText(docFilter, false, false, caseSensitive) + " );";
  query.prepare(queryText);
  if (queryText.contains(":labelid")) {
    query.bindValue(":labelid", filterLabelId);
  }
  query.bindValue(":pat", pattern);
}

void DocListModel::adjustQuery(DocFilter newDocFilter, int newFilterLabelId,
                               const QString& newSearchPattern, int newLimit,
                               int newOffset) {
  auto needRefreshNDocs =
      nDocsCurrentQuery_ == -1 || newDocFilter != docFilter_ ||
      newFilterLabelId != filterLabelId_ || newSearchPattern != searchPattern_;
  limit_ = newLimit;
  offset_ = newOffset;
  docFilter_ = newDocFilter;
  filterLabelId_ = newFilterLabelId;
  searchPattern_ = newSearchPattern;
  resultSetOutdated_ = false;

  auto query = getQuery();
  prepareQuery(query, newDocFilter, newFilterLabelId, newSearchPattern,
               newLimit, newOffset);
  query.exec();
  assert(query.isActive());
  if (needRefreshNDocs) {
    refreshNDocsCurrentQuery();
  }
  setQuery(query);
}

bool DocListModel::shouldBeCaseSensitive(const QString& searchPattern) {
  if (!searchPattern.isLower()) {
    return true;
  }
  if (QRegExp{R"(\s*".*"\s*)"}.exactMatch(searchPattern)) {
    return true;
  }
  if (QRegExp{R"(\s*'.*'\s*)"}.exactMatch(searchPattern)) {
    return true;
  }
  return false;
}

QString DocListModel::transformSearchPattern(const QString& searchPattern) {
  auto newPattern = searchPattern.trimmed();
  newPattern.replace(QRegExp{R"-(^"(.*)"$)-"}, R"(\1)");
  newPattern.replace(QRegExp{R"(^'(.*)'$)"}, R"(\1)");
  return newPattern;
}

QString DocListModel::transformLikePattern(const QString& searchPattern) {
  if (searchPattern.isEmpty()) {
    return "%";
  }
  QString newPattern{searchPattern};
  newPattern.replace(R"(\)", R"(\\)");
  newPattern.replace("%", R"(\%)");
  newPattern.replace("_", R"(\_)");
  return QString{"%%1%"}.arg(newPattern);
}

int DocListModel::nDocsCurrentQuery() {
  if (nDocsCurrentQuery_ == -1) {
    refreshNDocsCurrentQuery();
  }
  return nDocsCurrentQuery_;
}

int DocListModel::totalNDocs(DocFilter docFilter, int filterLabelId,
                             const QString& searchPattern) {
  auto query = getQuery();
  if (!searchPattern.trimmed().isEmpty()) {
    prepareCountQuery(query, docFilter, filterLabelId, searchPattern);
    query.exec();
    query.next();
    return query.value(0).toInt();
  }
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

void DocListModel::refreshNDocsCurrentQuery() {
  nDocsCurrentQuery_ = totalNDocs(docFilter_, filterLabelId_, searchPattern_);
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
  nDocsCurrentQuery_ = -1;
  adjustQuery(docFilter_, filterLabelId_, searchPattern_, limit_, offset_);
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
