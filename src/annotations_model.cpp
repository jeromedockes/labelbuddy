#include <cassert>

#include <QObject>
#include <QSqlDatabase>
#include <QVariant>

#include "annotations_model.h"

namespace labelbuddy {

AnnotationsModel::AnnotationsModel(QObject* parent) : QObject(parent) {}

QSqlQuery AnnotationsModel::get_query() const {
  return QSqlQuery(QSqlDatabase::database(database_name_));
}

void AnnotationsModel::set_database(const QString& new_database_name) {
  assert(QSqlDatabase::contains(new_database_name));
  database_name_ = new_database_name;
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
  query.prepare("select coalesce(display_title, '') "
                "as title from document where id = :docid ;");
  query.bindValue(":docid", current_doc_id_);
  query.exec();
  if (query.next()) {
    return query.value(0).toString();
  }
  return QString("");

}

QString AnnotationsModel::get_content() const {
  auto query = get_query();
  query.prepare("select content from document where id = :docid ;");
  query.bindValue(":docid", current_doc_id_);
  query.exec();
  if (query.next()) {
    return query.value(0).toString();
  }
  return QString("");
}

void AnnotationsModel::fill_index_converters(const QString& text) {
  surrogate_indices_in_qstring_.clear();
  surrogate_indices_in_unicode_string_.clear();
  int q_pos{};
  int u_pos{};
  for (auto qchar : text) {
    if (!qchar.isSurrogate()) {
      ++q_pos;
      ++u_pos;
    } else if (qchar.isHighSurrogate()) {
      surrogate_indices_in_qstring_ << q_pos;
      ++q_pos;
      surrogate_indices_in_unicode_string_ << u_pos;
      ++u_pos;
    } else {
      assert(qchar.isLowSurrogate());
      ++q_pos;
    }
  }
}

int AnnotationsModel::code_point_idx_to_utf16_idx(int cp_idx) const {
  assert(cp_idx >= 0);
  int utf_idx = cp_idx;
  auto it = surrogate_indices_in_unicode_string_.cbegin();
  while (it != surrogate_indices_in_unicode_string_.cend() && *it < cp_idx) {
    ++utf_idx;
    ++it;
  }
  return utf_idx;
}

int AnnotationsModel::utf16_idx_to_code_point_idx(int utf16_idx) const {
  assert(utf16_idx >= 0);
  int cp_idx = utf16_idx;
  auto it = surrogate_indices_in_qstring_.cbegin();
  while (it != surrogate_indices_in_qstring_.cend() && *it < utf16_idx) {
    --cp_idx;
    ++it;
  }
  assert(utf16_idx >= 0);
  return cp_idx;
}

QMap<int, LabelInfo> AnnotationsModel::get_labels_info() const {
  auto query = get_query();
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

QMap<int, AnnotationInfo> AnnotationsModel::get_annotations_info() const {
  auto query = get_query();
  query.prepare("select rowid, label_id, start_char, end_char, extra_data from "
                "annotation where doc_id = :doc order by rowid;");
  query.bindValue(":doc", current_doc_id_);
  query.exec();
  QMap<int, AnnotationInfo> result{};
  while (query.next()) {
    result[query.value(0).toInt()] =
        AnnotationInfo{query.value(0).toInt(), query.value(1).toInt(),
                       code_point_idx_to_utf16_idx(query.value(2).toInt()),
                       code_point_idx_to_utf16_idx(query.value(3).toInt()),
                       query.value(4).toString()};
  }
  return result;
}

int AnnotationsModel::add_annotation(int label_id, int start_char,
                                     int end_char) {
  auto query = get_query();
  query.prepare("insert into annotation (doc_id, label_id, start_char, "
                "end_char) values (:doc, :label, :start, :end);");
  query.bindValue(":doc", current_doc_id_);
  query.bindValue(":label", label_id);
  query.bindValue(":start", utf16_idx_to_code_point_idx(start_char));
  query.bindValue(":end", utf16_idx_to_code_point_idx(end_char));
  if (!query.exec()) {
    // fails eg if annotation is a duplicate of one already in db
    return -1;
  }
  auto new_annotation_id = query.lastInsertId().toInt();
  query.prepare("select count(*) from annotation where doc_id = :doc;");
  query.bindValue(":doc", current_doc_id_);
  query.exec();
  query.next();
  if (query.value(0).toInt() == 1) {
    emit document_status_changed(DocumentStatus::Labelled);
    emit document_gained_label(label_id, current_doc_id_);
  } else {
    query.prepare("select count(*) from annotation where doc_id = :doc and "
                  "label_id = :label;");
    query.bindValue(":doc", current_doc_id_);
    query.bindValue(":label", label_id);
    query.exec();
    query.next();
    if (query.value(0).toInt() == 1) {
      emit document_gained_label(label_id, current_doc_id_);
    }
  }
  return new_annotation_id;
}

int AnnotationsModel::delete_annotation(int annotation_id) {
  auto query = get_query();
  query.prepare("select label_id from annotation where rowid = :id;");
  query.bindValue(":id", annotation_id);
  query.exec();
  query.next();
  auto label_id = query.value(0).toInt();
  query.prepare("delete from annotation where rowid = :id;");
  query.bindValue(":id", annotation_id);
  query.exec();
  auto n_deleted = query.numRowsAffected();
  assert(n_deleted == 1);
  // -1 if query is not active
  if (n_deleted <= 0) {
    return 0;
  }
  query.prepare("select count(*) from annotation where doc_id = :doc;");
  query.bindValue(":doc", current_doc_id_);
  query.exec();
  query.next();
  if (query.value(0).toInt() == 0) {
    emit document_status_changed(DocumentStatus::Unlabelled);
    emit document_lost_label(label_id, current_doc_id_);
  } else {
    query.prepare("select count(*) from annotation where doc_id = :doc and "
                  "label_id = :label;");
    query.bindValue(":doc", current_doc_id_);
    query.bindValue(":label", label_id);
    query.exec();
    query.next();
    if (query.value(0).toInt() == 0) {
      emit document_lost_label(label_id, current_doc_id_);
    }
  }
  return n_deleted;
}

bool AnnotationsModel::update_annotation_extra_data(int annotation_id,
                                                    const QString& new_data) {
  auto query = get_query();
  query.prepare("update annotation set extra_data = :data where rowid = :id;");
  query.bindValue(":data", new_data == "" ? QVariant() : new_data);
  query.bindValue(":id", annotation_id);
  return query.exec();
}

void AnnotationsModel::check_current_doc() {
  auto query = get_query();
  query.prepare("select count(*) from document where id = :doc;");
  query.bindValue(":doc", current_doc_id_);
  query.exec();
  query.next();
  if (query.value(0) != 1) {
    visit_first_doc();
  }
}

void AnnotationsModel::visit_first_doc() {
  auto success = visit_query_result("select min(id) from document;");
  if (!success) {
    visit_doc(-1);
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
  query.bindValue(":doc", current_doc_id_);
  query.exec();
  query.next();
  if (query.isNull(0)) {
    return false;
  }
  visit_doc(query.value(0).toInt());
  return true;
}

void AnnotationsModel::visit_doc(int doc_id) {
  current_doc_id_ = doc_id;
  if (doc_id == -1) {
    fill_index_converters("");
  } else {
    auto query = get_query();
    query.prepare("update app_state set last_visited_doc = :doc;");
    query.bindValue(":doc", doc_id);
    query.exec();
    fill_index_converters(get_content());
  }
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

bool AnnotationsModel::is_positioned_on_valid_doc() const {
  return current_doc_id_ != -1;
}

int AnnotationsModel::current_doc_position() const {
  auto query = get_query();
  query.prepare("select count(*) from document where id < :docid;");
  query.bindValue(":docid", current_doc_id_);
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
  auto res = get_query_result("select max(id) from unlabelled_document;");
  return res;
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
  return current_doc_id_ < last_doc_id();
}

bool AnnotationsModel::has_prev() const {
  auto first = first_doc_id();
  return (first != -1) && (current_doc_id_ > first);
}

bool AnnotationsModel::has_next_labelled() const {
  return current_doc_id_ < last_labelled_doc_id();
}

bool AnnotationsModel::has_prev_labelled() const {

  auto first = first_labelled_doc_id();
  return (first != -1) && (current_doc_id_ > first);
}

bool AnnotationsModel::has_next_unlabelled() const {
  return current_doc_id_ < last_unlabelled_doc_id();
}

bool AnnotationsModel::has_prev_unlabelled() const {
  auto first = first_unlabelled_doc_id();
  return (first != -1) && (current_doc_id_ > first);
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
