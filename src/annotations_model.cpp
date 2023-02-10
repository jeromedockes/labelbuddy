#include <cassert>

#include <QObject>
#include <QSqlDatabase>
#include <QVariant>

#include "annotations_model.h"

namespace labelbuddy {

AnnotationsModel::AnnotationsModel(QObject* parent) : QObject(parent) {}

QSqlQuery AnnotationsModel::getQuery() const {
  return QSqlQuery(QSqlDatabase::database(databaseName_));
}

void AnnotationsModel::setDatabase(const QString& newDatabaseName) {
  assert(QSqlDatabase::contains(newDatabaseName));
  databaseName_ = newDatabaseName;
  auto query = getQuery();
  query.exec("select last_visited_doc from app_state;");
  query.next();
  if (query.value(0) == QVariant()) {
    visitFirstDoc();
    return;
  }
  auto lastVisited = query.value(0).toInt();
  query.prepare("select count(*) from document where id = :doc;");
  query.bindValue(":doc", lastVisited);
  query.exec();
  query.next();
  auto exists = query.value(0).toInt();
  if (exists) {
    visitDoc(lastVisited);
  } else {
    visitFirstDoc();
  }
}

QString AnnotationsModel::getTitle() const {
  auto query = getQuery();
  query.prepare("select coalesce(display_title, '') "
                "as title from document where id = :docid ;");
  query.bindValue(":docid", currentDocId_);
  query.exec();
  if (query.next()) {
    return query.value(0).toString();
  }
  return QString("");
}

QString AnnotationsModel::getContent() const { return text_; }

int AnnotationsModel::qStringIdxToUnicodeIdx(int qStringIndex) const {
  return charIndices_.qStringToUnicode(qStringIndex);
}

int AnnotationsModel::unicodeIdxToQStringIdx(int unicodeIndex) const {
  return charIndices_.unicodeToQString(unicodeIndex);
}

QMap<int, LabelInfo> AnnotationsModel::getLabelsInfo() const {
  auto query = getQuery();
  query.prepare("select id, color, name from sorted_label;");
  query.exec();
  QMap<int, LabelInfo> result{};
  while (query.next()) {
    result[query.value(0).toInt()] =
        LabelInfo{query.value(0).toInt(), query.value(1).toString(),
                  query.value(2).toString()};
  }
  return result;
}

QMap<int, AnnotationInfo> AnnotationsModel::getAnnotationsInfo() const {
  auto query = getQuery();
  query.prepare("select rowid, label_id, start_char, end_char, extra_data from "
                "annotation where doc_id = :doc order by rowid;");
  query.bindValue(":doc", currentDocId_);
  query.exec();
  QMap<int, AnnotationInfo> result{};
  while (query.next()) {
    result[query.value(0).toInt()] =
        AnnotationInfo{query.value(0).toInt(), query.value(1).toInt(),
                       unicodeIdxToQStringIdx(query.value(2).toInt()),
                       unicodeIdxToQStringIdx(query.value(3).toInt()),
                       query.value(4).toString()};
  }
  return result;
}

int AnnotationsModel::addAnnotation(int labelId, int startChar, int endChar) {
  auto query = getQuery();
  query.prepare("insert into annotation (doc_id, label_id, start_char, "
                "end_char) values (:doc, :label, :start, :end);");
  query.bindValue(":doc", currentDocId_);
  query.bindValue(":label", labelId);
  query.bindValue(":start", qStringIdxToUnicodeIdx(startChar));
  query.bindValue(":end", qStringIdxToUnicodeIdx(endChar));
  if (!query.exec()) {
    // fails eg if annotation is a duplicate of one already in db
    return -1;
  }
  auto newAnnotationId = query.lastInsertId().toInt();
  emit annotationAdded({newAnnotationId, labelId, startChar, endChar, ""});
  query.prepare("select count(*) from annotation where doc_id = :doc;");
  query.bindValue(":doc", currentDocId_);
  query.exec();
  query.next();
  if (query.value(0).toInt() == 1) {
    emit documentStatusChanged(DocumentStatus::Labelled);
    emit documentGainedLabel(labelId, currentDocId_);
  } else {
    query.prepare("select count(*) from annotation where doc_id = :doc and "
                  "label_id = :label;");
    query.bindValue(":doc", currentDocId_);
    query.bindValue(":label", labelId);
    query.exec();
    query.next();
    if (query.value(0).toInt() == 1) {
      emit documentGainedLabel(labelId, currentDocId_);
    }
  }
  return newAnnotationId;
}

int AnnotationsModel::deleteAnnotation(int annotationId) {
  auto query = getQuery();
  query.prepare("select label_id from annotation where rowid = :id;");
  query.bindValue(":id", annotationId);
  query.exec();
  query.next();
  auto labelId = query.value(0).toInt();
  query.prepare("delete from annotation where rowid = :id;");
  query.bindValue(":id", annotationId);
  query.exec();
  auto nDeleted = query.numRowsAffected();
  assert(nDeleted == 1);
  // -1 if query is not active
  if (nDeleted <= 0) {
    return 0;
  }
  emit annotationDeleted(annotationId);
  query.prepare("select count(*) from annotation where doc_id = :doc;");
  query.bindValue(":doc", currentDocId_);
  query.exec();
  query.next();
  if (query.value(0).toInt() == 0) {
    emit documentStatusChanged(DocumentStatus::Unlabelled);
    emit documentLostLabel(labelId, currentDocId_);
  } else {
    query.prepare("select count(*) from annotation where doc_id = :doc and "
                  "label_id = :label;");
    query.bindValue(":doc", currentDocId_);
    query.bindValue(":label", labelId);
    query.exec();
    query.next();
    if (query.value(0).toInt() == 0) {
      emit documentLostLabel(labelId, currentDocId_);
    }
  }
  return nDeleted;
}

bool AnnotationsModel::updateAnnotationExtraData(int annotationId,
                                                 const QString& newData) {
  auto query = getQuery();
  query.prepare("update annotation set extra_data = :data where rowid = :id;");
  query.bindValue(":data", newData == "" ? QVariant() : newData);
  query.bindValue(":id", annotationId);
  auto success = query.exec();
  if (! success){
    return success;
  }
  emit extraDataChanged(annotationId, newData);
  return success;
}

void AnnotationsModel::checkCurrentDoc() {
  auto query = getQuery();
  query.prepare("select count(*) from document where id = :doc;");
  query.bindValue(":doc", currentDocId_);
  query.exec();
  query.next();
  if (query.value(0) != 1) {
    visitFirstDoc();
  }
}

void AnnotationsModel::visitFirstDoc() {
  auto success = visitQueryResult("select min(id) from document;");
  if (!success) {
    visitDoc(-1);
  }
}

void AnnotationsModel::visitNext() {
  visitQueryResult("select min(id) from document where id > :doc;");
}

void AnnotationsModel::visitPrev() {
  visitQueryResult("select max(id) from document where id < :doc;");
}

void AnnotationsModel::visitNextLabelled() {
  visitQueryResult("select min(id) from labelled_document where id > :doc;");
}

void AnnotationsModel::visitPrevLabelled() {
  visitQueryResult("select max(id) from labelled_document where id < :doc;");
}

void AnnotationsModel::visitNextUnlabelled() {
  visitQueryResult("select min(id) from unlabelled_document where id > :doc;");
}

void AnnotationsModel::visitPrevUnlabelled() {
  visitQueryResult("select max(id) from unlabelled_document where id < :doc;");
}

bool AnnotationsModel::visitQueryResult(const QString& queryText) {
  auto query = getQuery();
  query.prepare(queryText);
  query.bindValue(":doc", currentDocId_);
  query.exec();
  query.next();
  if (query.isNull(0)) {
    return false;
  }
  visitDoc(query.value(0).toInt());
  return true;
}

void AnnotationsModel::updateText() {
  auto query = getQuery();
  query.prepare("select content from document where id = :docid ;");
  query.bindValue(":docid", currentDocId_);
  query.exec();
  if (query.next()) {
    text_ = query.value(0).toString();
  } else {
    text_ = "";
  }
  charIndices_.setText(text_);
}

void AnnotationsModel::visitDoc(int docId) {
  currentDocId_ = docId;
  if (docId == -1) {
    text_ = "";
    charIndices_.setText("");
  } else {
    auto query = getQuery();
    query.prepare("update app_state set last_visited_doc = :doc;");
    query.bindValue(":doc", docId);
    query.exec();
    updateText();
  }
  emit documentChanged();
}

int AnnotationsModel::getQueryResult(const QString& queryText) const {
  auto query = getQuery();
  query.prepare(queryText);
  query.exec();
  query.next();
  if (query.isNull(0)) {
    return -1;
  }
  return query.value(0).toInt();
}

bool AnnotationsModel::isPositionedOnValidDoc() const {
  return currentDocId_ != -1;
}

int AnnotationsModel::currentDocPosition() const {
  auto query = getQuery();
  query.prepare("select count(*) from document where id < :docid;");
  query.bindValue(":docid", currentDocId_);
  query.exec();
  query.next();
  return query.value(0).toInt();
}

int AnnotationsModel::lastDocId() const {
  return getQueryResult("select max(id) from document;");
}
int AnnotationsModel::firstDocId() const {

  return getQueryResult("select min(id) from document;");
}
int AnnotationsModel::lastUnlabelledDocId() const {
  auto res = getQueryResult("select max(id) from unlabelled_document;");
  return res;
}
int AnnotationsModel::firstUnlabelledDocId() const {

  return getQueryResult("select min(id) from unlabelled_document;");
}
int AnnotationsModel::lastLabelledDocId() const {

  return getQueryResult("select max(doc_id) from annotation;");
}
int AnnotationsModel::firstLabelledDocId() const {
  return getQueryResult("select min(doc_id) from annotation;");
}

int AnnotationsModel::totalNDocs() const {
  return getQueryResult("select count(*) from document;");
}

bool AnnotationsModel::hasNext() const { return currentDocId_ < lastDocId(); }

bool AnnotationsModel::hasPrev() const {
  auto first = firstDocId();
  return (first != -1) && (currentDocId_ > first);
}

bool AnnotationsModel::hasNextLabelled() const {
  return currentDocId_ < lastLabelledDocId();
}

bool AnnotationsModel::hasPrevLabelled() const {

  auto first = firstLabelledDocId();
  return (first != -1) && (currentDocId_ > first);
}

bool AnnotationsModel::hasNextUnlabelled() const {
  return currentDocId_ < lastUnlabelledDocId();
}

bool AnnotationsModel::hasPrevUnlabelled() const {
  auto first = firstUnlabelledDocId();
  return (first != -1) && (currentDocId_ > first);
}

int AnnotationsModel::shortcutToId(const QString& shortcut) const {
  auto query = getQuery();
  query.prepare("select id from label where shortcut_key = :key;");
  query.bindValue(":key", shortcut);
  query.exec();
  if (!query.next()) {
    return -1;
  }
  return query.value(0).toInt();
}
} // namespace labelbuddy
