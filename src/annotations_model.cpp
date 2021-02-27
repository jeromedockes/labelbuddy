#include <QObject>
#include <QSqlDatabase>
#include <QVariant>

#include "annotations_model.h"

namespace labelbuddy {

AnnotationsModel::AnnotationsModel(QObject* parent) : QObject(parent) {}

QSqlQuery AnnotationsModel::get_query() const {
  return QSqlQuery(QSqlDatabase::database(database_name));
}

void AnnotationsModel::set_database(const QString& new_database_name) {
  database_name = new_database_name;
  auto query = get_query();
  query.exec("select last_visited_doc from app_state;");
  query.next();
  if (query.value(0) == QVariant()) {
    visit_first_doc();
    return;
  }
  auto last_visited = query.value(0).toInt();
  query.prepare("select count(*) from document where id = :doc;");
  query.bindValue(":doc", last_visited);
  query.exec();
  query.next();
  auto exists = query.value(0).toInt();
  if (exists) {
    visit_doc(last_visited);
  } else {
    visit_first_doc();
  }
}

QString AnnotationsModel::get_title() const {
  auto query = get_query();
  query.prepare("select coalesce(short_title, title, '') "
                "as title from document where id = :docid ;");
  query.bindValue(":docid", current_doc_id);
  query.exec();
  if (query.next()) {
    return query.value(0).toString();
  } else {
    return QString("");
  }
}

QString AnnotationsModel::get_content() const {
  auto query = get_query();
  query.prepare("select content from document where id = :docid ;");
  query.bindValue(":docid", current_doc_id);
  query.exec();
  if (query.next()) {
    return query.value(0).toString();
  } else {
    return QString("");
  }
}

QMap<int, LabelInfo> AnnotationsModel::get_labels_info() const {
  auto query = get_query();
  query.prepare("select id, color from label order by id;");
  query.exec();
  QMap<int, LabelInfo> result{};
  while (query.next()) {
    result[query.value(0).toInt()] =
        LabelInfo{query.value(0).toInt(), query.value(1).toString()};
  }
  return result;
}

QMap<int, AnnotationInfo> AnnotationsModel::get_annotations_info() const {
  auto query = get_query();
  query.prepare("select rowid, label_id, start_char, end_char from annotation "
                "where doc_id = :doc order by rowid;");
  query.bindValue(":doc", current_doc_id);
  query.exec();
  QMap<int, AnnotationInfo> result{};
  while (query.next()) {
    result[query.value(0).toInt()] =
        AnnotationInfo{query.value(0).toInt(), query.value(1).toInt(),
                       query.value(2).toInt(), query.value(3).toInt()};
  }
  return result;
}

int AnnotationsModel::add_annotation(int label_id, int start_char,
                                     int end_char) {
  auto query = get_query();
  query.prepare("insert into annotation (doc_id, label_id, start_char, "
                "end_char) values (:doc, :label, :start, :end);");
  query.bindValue(":doc", current_doc_id);
  query.bindValue(":label", label_id);
  query.bindValue(":start", start_char);
  query.bindValue(":end", end_char);
  if (!query.exec()) {
    return -1;
  }
  auto new_annotation_id = query.lastInsertId().toInt();
  query.prepare("select count(*) from annotation where doc_id = :doc;");
  query.bindValue(":doc", current_doc_id);
  query.exec();
  query.next();
  if (query.value(0).toInt() == 1) {
    emit document_status_changed();
  }
  return new_annotation_id;
}

void AnnotationsModel::delete_annotations(QList<int> annotation_ids) {
  QVariantList ids{};
  for (auto id : annotation_ids) {
    ids << id;
  }
  auto query = get_query();
  query.prepare("delete from annotation where rowid = :id;");
  query.bindValue(":id", ids);
  query.execBatch();
  query.prepare("select count(*) from annotation where doc_id = :doc;");
  query.bindValue(":doc", current_doc_id);
  query.exec();
  query.next();
  if (query.value(0).toInt() == 0) {
    emit document_status_changed();
  }
  emit annotations_changed();
}

void AnnotationsModel::check_current_doc() {
  auto query = get_query();
  query.prepare("select count(*) from document where id = :doc;");
  query.bindValue(":doc", current_doc_id);
  query.exec();
  query.next();
  if (query.value(0) != 1) {
    visit_first_doc();
  }
}

void AnnotationsModel::visit_first_doc() {
  auto success = visit_query_result("select min(id) from document;");
  if (!success) {
    current_doc_id = -1;
    emit document_changed();
  }
}

void AnnotationsModel::visit_next() {
  visit_query_result("select min(id) from document where id > :doc;");
}

void AnnotationsModel::visit_prev() {
  visit_query_result("select max(id) from document where id < :doc;");
}

void AnnotationsModel::visit_next_labelled() {
  visit_query_result("select min(id) from labelled_document where id > :doc;");
}

void AnnotationsModel::visit_prev_labelled() {
  visit_query_result("select max(id) from labelled_document where id < :doc;");
}

void AnnotationsModel::visit_next_unlabelled() {
  visit_query_result(
      "select min(id) from unlabelled_document where id > :doc;");
}

void AnnotationsModel::visit_prev_unlabelled() {
  visit_query_result(
      "select max(id) from unlabelled_document where id < :doc;");
}

bool AnnotationsModel::visit_query_result(const QString& query_text) {
  auto query = get_query();
  query.prepare(query_text);
  query.bindValue(":doc", current_doc_id);
  query.exec();
  query.next();
  if (query.isNull(0)) {
    return false;
  }
  visit_doc(query.value(0).toInt());
  return true;
}

void AnnotationsModel::visit_doc(int doc_id) {
  current_doc_id = doc_id;
  auto query = get_query();
  query.prepare("update app_state set last_visited_doc = :doc;");
  query.bindValue(":doc", doc_id);
  query.exec();
  emit document_changed();
}

int AnnotationsModel::get_query_result(const QString& query_text) const {
  auto query = get_query();
  query.prepare(query_text);
  query.exec();
  query.next();
  if (query.isNull(0)) {
    return -1;
  }
  return query.value(0).toInt();
}

int AnnotationsModel::current_doc_position() const {
  auto query = get_query();
  query.prepare("select count(*) from document where id < :docid;");
  query.bindValue(":docid", current_doc_id);
  query.exec();
  query.next();
  return query.value(0).toInt();
}

int AnnotationsModel::last_doc_id() const {
  return get_query_result("select max(id) from document;");
}
int AnnotationsModel::first_doc_id() const {

  return get_query_result("select min(id) from document;");
}
int AnnotationsModel::last_unlabelled_doc_id() const {

  return get_query_result("select max(id) from unlabelled_document;");
}
int AnnotationsModel::first_unlabelled_doc_id() const {

  return get_query_result("select min(id) from unlabelled_document;");
}
int AnnotationsModel::last_labelled_doc_id() const {

  return get_query_result("select max(doc_id) from annotation;");
}
int AnnotationsModel::first_labelled_doc_id() const {
  return get_query_result("select min(doc_id) from annotation;");
}

int AnnotationsModel::total_n_docs() const {
  return get_query_result("select count(*) from document;");
}

bool AnnotationsModel::has_next() const {
  return current_doc_id < last_doc_id();
}

bool AnnotationsModel::has_prev() const {
  auto first = first_doc_id();
  return (first != -1) && (current_doc_id > first);
}

bool AnnotationsModel::has_next_labelled() const {
  return current_doc_id < last_labelled_doc_id();
}

bool AnnotationsModel::has_prev_labelled() const {

  auto first = first_labelled_doc_id();
  return (first != -1) && (current_doc_id > first);
}

bool AnnotationsModel::has_next_unlabelled() const {
  return current_doc_id < last_unlabelled_doc_id();
}

bool AnnotationsModel::has_prev_unlabelled() const {
  auto first = first_unlabelled_doc_id();
  return (first != -1) && (current_doc_id > first);
}

int AnnotationsModel::shortcut_to_id(const QString& shortcut) const {
  auto query = get_query();
  query.prepare("select id from label where shortcut_key = :key;");
  query.bindValue(":key", shortcut);
  query.exec();
  if (!query.next()) {
    return -1;
  }
  return query.value(0).toInt();
}
} // namespace labelbuddy
