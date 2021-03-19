#include <iostream>
#include <memory>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QString>
#include <QXmlStreamWriter>

#include "database.h"
#include "utils.h"

namespace labelbuddy {

// about encoding: when reading .txt we use the Qt default (depends on locale)
// when reading or writing json (or jsonl) we force utf-8 (json is always utf-8)
// when reading xml the encoding should specified in the xml prolog
// when writing xml we force utf-8 (which is the Qt default on all platforms)

DocsReader::DocsReader(const QString& file_path) : file(file_path) {
  file.open(QIODevice::ReadOnly | QIODevice::Text);
}

DocsReader::~DocsReader() {}

bool DocsReader::read_next() { return false; }

bool DocsReader::is_open() const { return file.isOpen(); }

int DocsReader::progress_max() const { return file.size(); }

int DocsReader::current_progress() const { return file.pos(); }

const DocRecord* DocsReader::get_current_record() const {
  return current_record.get();
}

QFile* DocsReader::get_file() { return &file; }

void DocsReader::set_current_record(std::unique_ptr<DocRecord> new_record) {
  current_record = std::move(new_record);
}

TxtDocsReader::TxtDocsReader(const QString& file_path)
    : DocsReader(file_path), stream(get_file()) {}

bool TxtDocsReader::read_next() {
  if (!is_open() || stream.atEnd()) {
    return false;
  }
  std::unique_ptr<DocRecord> new_record(new DocRecord);
  new_record->content = stream.readLine();
  set_current_record(std::move(new_record));
  return true;
}

XmlDocsReader::XmlDocsReader(const QString& file_path)
    : DocsReader(file_path), xml(get_file()) {
  if (!is_open()) {
    found_error = true;
    return;
  }
  xml.readNextStartElement();
  if (xml.name() != "document_set") {
    found_error = true;
  }
}

bool XmlDocsReader::read_next() {
  if (!is_open() || found_error) {
    return false;
  }
  if (!xml.readNextStartElement() || xml.name() != "document") {
    found_error = true;
    return false;
  }
  new_record.reset(new DocRecord);
  new_record->valid_content = false;
  read_document();
  if (found_error) {
    return false;
  }
  set_current_record(std::move(new_record));
  new_record.reset();
  return true;
}

void XmlDocsReader::read_document() {
  while (xml.readNextStartElement()) {
    if (xml.name() == "text") {
      read_document_text();
    } else if (xml.name() == "document_md5_checksum") {
      read_md5();
    } else if (xml.name() == "meta") {
      read_meta();
    } else if (xml.name() == "title") {
      read_title();
    } else if (xml.name() == "short_title") {
      read_short_title();
    } else if (xml.name() == "long_title") {
      read_long_title();
    } else if (xml.name() == "labels") {
      read_annotation_set();
    } else {
      xml.skipCurrentElement();
    }
  }
}

void XmlDocsReader::read_document_text() {
  new_record->content =
      xml.readElementText(QXmlStreamReader::IncludeChildElements);
  new_record->valid_content = true;
}

void XmlDocsReader::read_md5() {
  new_record->declared_md5 =
      xml.readElementText(QXmlStreamReader::IncludeChildElements);
}

void XmlDocsReader::read_title() {
  new_record->title =
      xml.readElementText(QXmlStreamReader::IncludeChildElements);
}
void XmlDocsReader::read_short_title() {
  new_record->short_title =
      xml.readElementText(QXmlStreamReader::IncludeChildElements);
}
void XmlDocsReader::read_long_title() {
  new_record->long_title =
      xml.readElementText(QXmlStreamReader::IncludeChildElements);
}

void XmlDocsReader::read_meta() {
  QJsonObject json{};
  for (const auto& attrib : xml.attributes()) {
    json[attrib.name().toString()] = attrib.value().toString();
  }
  new_record->extra_data = QJsonDocument(json).toJson(QJsonDocument::Compact);
  xml.readElementText();
}

void XmlDocsReader::read_annotation_set() {
  while (xml.readNextStartElement()) {
    if (xml.name() == "annotation") {
      read_annotation();
    } else {
      xml.skipCurrentElement();
    }
  }
}

void XmlDocsReader::read_annotation() {
  QJsonArray annotation{};
  xml.readNextStartElement();
  if (xml.name() != "start_char") {
    found_error = true;
    return;
  }
  annotation << xml.readElementText().toInt();
  xml.readNextStartElement();
  if (xml.name() != "end_char") {
    found_error = true;
    return;
  }
  annotation << xml.readElementText().toInt();
  xml.readNextStartElement();
  if (xml.name() != "label") {
    found_error = true;
    return;
  }
  annotation << xml.readElementText();
  new_record->annotations << annotation;
  xml.skipCurrentElement();
}

std::unique_ptr<DocRecord> json_to_doc_record(const QJsonDocument& json) {
  if (json.isArray()) {
    return json_to_doc_record(json.array());
  } else {
    return json_to_doc_record(json.object());
  }
}

std::unique_ptr<DocRecord> json_to_doc_record(const QJsonValue& json) {
  if (json.isArray()) {
    return json_to_doc_record(json.toArray());
  } else {
    return json_to_doc_record(json.toObject());
  }
}

std::unique_ptr<DocRecord> json_to_doc_record(const QJsonObject& json) {
  std::unique_ptr<DocRecord> record(new DocRecord);
  if (json.contains("text")) {
    record->content = json["text"].toString();
  } else {
    record->valid_content = false;
  }
  record->declared_md5 = json["document_md5_checksum"].toString();
  record->annotations = json["labels"].toArray();
  record->extra_data =
      QJsonDocument(json["meta"].toObject()).toJson(QJsonDocument::Compact);
  record->title = json["title"].toString();
  record->short_title = json["short_title"].toString();
  record->long_title = json["long_title"].toString();
  return record;
}

std::unique_ptr<DocRecord> json_to_doc_record(const QJsonArray& json) {
  std::unique_ptr<DocRecord> record(new DocRecord);
  record->content = json[0].toString();
  record->extra_data =
      QJsonDocument(json[1].toObject()).toJson(QJsonDocument::Compact);
  return record;
}

JsonDocsReader::JsonDocsReader(const QString& file_path)
    : DocsReader(file_path) {
  if (!is_open()) {
    total_n_docs = 0;
    return;
  }
  QTextStream input_stream(get_file());
  input_stream.setCodec("UTF-8");
  all_docs = QJsonDocument::fromJson(input_stream.readAll().toUtf8()).array();
  total_n_docs = all_docs.size();
  current_doc = all_docs.constBegin();
}

bool JsonDocsReader::read_next() {
  if (total_n_docs == 0 || current_doc == all_docs.constEnd()) {
    return false;
  }
  set_current_record(json_to_doc_record(*current_doc));
  ++current_doc;
  ++n_seen;
  return true;
}

int JsonDocsReader::progress_max() const { return total_n_docs; }

int JsonDocsReader::current_progress() const { return n_seen; }

JsonLinesDocsReader::JsonLinesDocsReader(const QString& file_path)
    : DocsReader(file_path), stream(get_file()) {
  stream.setCodec("UTF-8");
}

bool JsonLinesDocsReader::read_next() {
  if (!is_open() || stream.atEnd()) {
    return false;
  }
  std::unique_ptr<DocRecord> new_record(new DocRecord);
  auto json_doc = QJsonDocument::fromJson(stream.readLine().toUtf8());
  set_current_record(json_to_doc_record(json_doc));
  return true;
}

DocsWriter::DocsWriter(const QString& file_path) : file(file_path) {
  file.open(QIODevice::WriteOnly | QIODevice::Text);
}

DocsWriter::~DocsWriter() {}

void DocsWriter::write_prefix() {}
void DocsWriter::write_suffix() {}

bool DocsWriter::is_open() const { return file.isOpen(); }

QFile* DocsWriter::get_file() { return &file; }

void DocsWriter::add_document(const QString& md5, const QVariant& content,
                              const QJsonObject& extra_data,
                              const QList<Annotation>* annotations,
                              const QString& user_name, const QString& title,
                              const QString& short_title,
                              const QString& long_title) {
  (void)md5;
  (void)content;
  (void)extra_data;
  (void)annotations;
  (void)user_name;
  (void)title;
  (void)short_title;
  (void)long_title;
}

DocsJsonLinesWriter::DocsJsonLinesWriter(const QString& file_path)
    : DocsWriter(file_path), stream(get_file()) {
  stream.setCodec("UTF-8");
}

int DocsJsonLinesWriter::get_n_docs() const { return n_docs; }

QTextStream& DocsJsonLinesWriter::get_stream() { return stream; }

void DocsJsonLinesWriter::add_document(
    const QString& md5, const QVariant& content, const QJsonObject& extra_data,
    const QList<Annotation>* annotations, const QString& user_name,
    const QString& title, const QString& short_title,
    const QString& long_title) {
  if (n_docs) {
    stream << "\n";
  }
  QJsonObject doc_json{};
  doc_json.insert("document_md5_checksum", md5);
  if (content != QVariant()) {
    doc_json.insert("text", content.toString());
  }
  doc_json.insert("meta", extra_data);
  if (user_name != QString()) {
    doc_json.insert("annotation_approver", user_name);
  }
  if (title != QString()) {
    doc_json.insert("title", title);
  }
  if (short_title != QString()) {
    doc_json.insert("short_title", short_title);
  }
  if (long_title != QString()) {
    doc_json.insert("long_title", long_title);
  }
  if (annotations != nullptr) {
    QJsonArray all_annotations_json{};
    for (const auto& annotation : *annotations) {
      QJsonArray annotation_json{};
      annotation_json.append(annotation.start_char);
      annotation_json.append(annotation.end_char);
      annotation_json.append(annotation.label_name);
      all_annotations_json.append(annotation_json);
    }
    doc_json.insert("labels", all_annotations_json);
  }
  stream << QString::fromUtf8(
      QJsonDocument(doc_json).toJson(QJsonDocument::Compact));
  ++n_docs;
}

void DocsJsonLinesWriter::write_suffix() {
  if (n_docs) {
    stream << "\n";
  }
}

DocsJsonWriter::DocsJsonWriter(const QString& file_path)
    : DocsJsonLinesWriter(file_path) {}

void DocsJsonWriter::add_document(const QString& md5, const QVariant& content,
                                  const QJsonObject& extra_data,
                                  const QList<Annotation>* annotations,
                                  const QString& user_name,
                                  const QString& title,
                                  const QString& short_title,
                                  const QString& long_title) {
  if (get_n_docs()) {
    get_stream() << ",";
  }
  DocsJsonLinesWriter::add_document(md5, content, extra_data, annotations,
                                    user_name, title, short_title, long_title);
}

void DocsJsonWriter::write_prefix() { get_stream() << "[\n"; }

void DocsJsonWriter::write_suffix() {
  get_stream() << (get_n_docs() ? "\n" : "") << "]\n";
}

DocsXmlWriter::DocsXmlWriter(const QString& file_path)
    : DocsWriter(file_path), xml(get_file()) {
  xml.setCodec("UTF-8");
  xml.setAutoFormatting(true);
}

void DocsXmlWriter::write_prefix() {
  xml.writeStartDocument();
  xml.writeStartElement("document_set");
}

void DocsXmlWriter::write_suffix() {
  xml.writeEndElement();
  xml.writeEndDocument();
}

void DocsXmlWriter::add_document(const QString& md5, const QVariant& content,
                                 const QJsonObject& extra_data,
                                 const QList<Annotation>* annotations,
                                 const QString& user_name, const QString& title,
                                 const QString& short_title,
                                 const QString& long_title) {

  xml.writeStartElement("document");
  if (content != QVariant()) {
    xml.writeTextElement("text", content.toString());
  }
  xml.writeTextElement("document_md5_checksum", md5);
  xml.writeStartElement("meta");
  for (auto key_val = extra_data.constBegin(); key_val != extra_data.constEnd();
       ++key_val) {
    xml.writeAttribute(key_val.key(), key_val.value().toVariant().toString());
  }
  xml.writeEndElement();
  if (user_name != QString()) {
    xml.writeTextElement("annotation_approver", user_name);
  }
  if (title != QString()) {
    xml.writeTextElement("title", title);
  }
  if (short_title != QString()) {
    xml.writeTextElement("short_title", short_title);
  }
  if (long_title != QString()) {
    xml.writeTextElement("long_title", long_title);
  }
  add_annotations(annotations);
  xml.writeEndElement();
}

void DocsXmlWriter::add_annotations(const QList<Annotation>* annotations) {
  if (annotations == nullptr) {
    return;
  }
  xml.writeStartElement("labels");
  for (const auto& annotation : *annotations) {
    xml.writeStartElement("annotation");
    xml.writeTextElement("start_char",
                         QString("%0").arg(annotation.start_char));
    xml.writeTextElement("end_char", QString("%0").arg(annotation.end_char));
    xml.writeTextElement("label", annotation.label_name);
    xml.writeEndElement();
  }
  xml.writeEndElement();
}

LabelRecord json_to_label_record(const QJsonValue& json) {
  LabelRecord record;
  if (json.isObject()) {
    auto json_obj = json.toObject();
    record.name = json_obj["text"].toString();
    if (json_obj.contains("color")) {
      record.color = json_obj["color"].toString();
    } else {
      record.color = json_obj["background_color"].toString();
    }
    if (json_obj.contains("shortcut_key")) {
      record.shortcut_key = json_obj["shortcut_key"].toString();
    } else {
      record.shortcut_key = json_obj["suffix_key"].toString();
    }
  } else {
    record.name = json.toArray()[0].toString();
    record.color = json.toArray()[1].toString();
  }
  return record;
}

QString DatabaseCatalog::get_default_database_path() {
  auto home_dir =
      QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0];
  auto default_path = QDir(home_dir).filePath("database.labelbuddy");
  QSettings settings("labelbuddy", "labelbuddy");
  if (!settings.contains("last_opened_database")) {
    return default_path;
  }
  auto last_open = settings.value("last_opened_database").toString();
  QFileInfo info(last_open);
  if (info.exists() && info.isWritable() && info.isReadable()) {
    return last_open;
  }
  return default_path;
}

QString DatabaseCatalog::check_database_path(const QString& database_path) {
  if (database_path == tmp_db_name_) {
    return database_path;
  }
  QString db_path{database_path};
  if (db_path == QString()) {
    db_path = get_default_database_path();
  }
  QFileInfo info(db_path);
  if (info.exists()) {
    if (info.isReadable() && info.isWritable()) {
      // we don't use canonicalFilePath so that symlinks are not resolved
      return info.absoluteFilePath();
    } else {
      return QString();
    }
  }
  QFile file(info.absoluteFilePath());
  bool opened = file.open(QIODevice::ReadWrite);
  if (opened) {
    file.close();
    return info.absoluteFilePath();
  } else {
    return QString();
  }
}

bool DatabaseCatalog::open_database(const QString& database_path) {
  QString actual_database_path{check_database_path(database_path)};
  if (actual_database_path == QString()) {
    return false;
  }
  if (databases.contains(actual_database_path)) {
    current_database = actual_database_path;
    return true;
  }
  // Qt's "database name" the param for sqlite call
  // it is the path to db file, or "" if temporary database.
  // the argument to addDatabase is the connection name
  // we choose the db file path for non-temporary and
  // ":LABELBUDDY_TEMPORARY_DATABASE:" otherwise
  auto db_name =
      actual_database_path == tmp_db_name_ ? "" : actual_database_path;
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", actual_database_path);
  db.setDatabaseName(db_name);
  if (!db.open()) {
    return false;
  }
  QSqlQuery query(db);
  if (!query.exec("PRAGMA foreign_keys = ON;")) {
    return false;
  }
  if (!create_tables(db)) {
    return false;
  }
  current_database = actual_database_path;
  databases.insert(actual_database_path);
  if (actual_database_path != tmp_db_name_) {
    QSettings settings("labelbuddy", "labelbuddy");
    settings.setValue("last_opened_database", current_database);
  }
  return true;
}

QString DatabaseCatalog::open_temp_database() {
  bool is_new = !databases.contains(tmp_db_name_);
  open_database(tmp_db_name_);
  if (is_new) {
    import_labels(":docs/example_data/example_labels.json");
    import_documents(":docs/example_data/example_documents.json");
  }
  return tmp_db_name_;
}

QString DatabaseCatalog::get_current_database() const {
  return current_database;
}

QString DatabaseCatalog::parent_directory(const QString& file_path) const {
  auto dir = QDir(file_path);
  dir.cdUp();
  return dir.absolutePath();
}

QString DatabaseCatalog::get_last_opened_directory() const {
  QSettings settings("labelbuddy", "labelbuddy");
  if (settings.contains("last_opened_database")) {
    return parent_directory(settings.value("last_opened_database").toString());
  }
  return QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0];
}

QVariant
DatabaseCatalog::get_app_state_extra(const QString& key,
                                     const QVariant& default_value) const {
  QSqlQuery query(QSqlDatabase::database(current_database));
  query.prepare("select value from app_state_extra where key = :key;");
  query.bindValue(":key", key);
  query.exec();
  if (query.next()) {
    return query.value(0);
  }
  return default_value;
}

void DatabaseCatalog::set_app_state_extra(const QString& key,
                                          const QVariant& value) {
  QSqlQuery query(QSqlDatabase::database(current_database));
  query.prepare("select count(*) from app_state_extra where key = :key;");
  query.bindValue(":key", key);
  query.exec();
  query.next();
  auto exists = query.value(0).toInt();
  if (exists) {
    query.prepare("update app_state_extra set value = :val where key = :key;");
    query.bindValue(":val", value);
    query.bindValue(":key", key);
    query.exec();
  } else {
    query.prepare(
        "insert into app_state_extra (key, value) values (:key, :val);");
    query.bindValue(":val", value);
    query.bindValue(":key", key);
    query.exec();
  }
}

int DatabaseCatalog::insert_doc_record(const DocRecord& record,
                                       QSqlQuery& query) {
  QByteArray hash{};
  if (record.valid_content) {
    query.prepare("insert into document (content, content_md5, extra_data, "
                  "title, short_title, long_title) values (:content, :md5, "
                  ":extra, :t, :st, :lt);");
    query.bindValue(":content", record.content);
    query.bindValue(":extra", record.extra_data);
    hash = QCryptographicHash::hash(record.content.toUtf8(),
                                    QCryptographicHash::Md5);
    query.bindValue(":md5", hash);
    query.bindValue(":t",
                    record.title != QString() ? record.title : QVariant());
    query.bindValue(":st", record.short_title != QString() ? record.short_title
                                                           : QVariant());
    query.bindValue(":lt", record.long_title != QString() ? record.long_title
                                                          : QVariant());
    query.exec();
  } else {
    if (record.declared_md5 == QString()) {
      return 0;
    }
    hash = QByteArray::fromHex(record.declared_md5.toUtf8());
  }
  if (record.annotations.size() == 0) {
    return 0;
  }
  query.prepare("select id from document where content_md5 = :md5;");
  query.bindValue(":md5", hash);
  query.exec();
  if (!query.next()) {
    return 0;
  }
  auto doc_id = query.value(0).toInt();
  int n_annotations{};
  for (const auto& annotation : record.annotations) {
    auto annotation_array = annotation.toArray();
    insert_label(query, annotation_array[2].toString());
    query.prepare("select id from label where name = :lname;");
    query.bindValue(":lname", annotation_array[2].toString());
    query.exec();
    query.next();
    auto label_id = query.value(0).toInt();
    query.prepare("insert into annotation (doc_id, label_id, start_char, "
                  "end_char) values (:docid, :labelid, :schar, :echar);");
    query.bindValue(":docid", doc_id);
    query.bindValue(":labelid", label_id);
    query.bindValue(":schar", annotation_array[0].toInt());
    query.bindValue(":echar", annotation_array[1].toInt());
    if (query.exec()) {
      ++n_annotations;
    }
  }
  return n_annotations;
}

void DatabaseCatalog::insert_label(QSqlQuery& query, const QString& label_name,
                                   const QString& color,
                                   const QString& shortcut_key) {
  auto re = shortcut_key_pattern();
  bool valid_shortcut = re.match(shortcut_key).hasMatch();
  if (valid_shortcut) {
    query.prepare("select id from label where keyboard_shortcut = :shortcut;");
    query.bindValue(":shortcut", shortcut_key);
    query.exec();
    if (query.next()) {
      valid_shortcut = false;
    }
  }
  query.prepare("insert into label (name, color, shortcut_key) values (:name, "
                ":color, :shortcut)");
  query.bindValue(":name", label_name);
  bool used_default_color{};
  if (QColor::isValidColor(color)) {
    query.bindValue(":color", QColor(color).name());
  } else {
    query.bindValue(":color",
                    label_colors[color_index % label_colors.length()]);
    used_default_color = true;
  }
  query.bindValue(":shortcut",
                  (valid_shortcut ? shortcut_key : QVariant(QVariant::String)));
  if (query.exec() && used_default_color) {
    ++color_index;
  }
}

QList<int> DatabaseCatalog::import_documents(const QString& file_path,
                                             QProgressDialog* progress) {
  QSqlQuery query(QSqlDatabase::database(current_database));
  query.exec("select count(*) from document;");
  query.next();
  auto n_before = query.value(0).toInt();

  std::unique_ptr<DocsReader> reader;
  auto suffix = QFileInfo(file_path).suffix();
  if (suffix == "xml") {
    reader.reset(new XmlDocsReader(file_path));
  } else if (suffix == "json") {
    reader.reset(new JsonDocsReader(file_path));
  } else if (suffix == "jsonl") {
    reader.reset(new JsonLinesDocsReader(file_path));
  } else {
    reader.reset(new TxtDocsReader(file_path));
  }

  if (progress != nullptr) {
    progress->setMaximum(reader->progress_max() + 1);
  }
  query.exec("begin transaction;");
  int n_annotations{};
  int n_docs_read{};
  std::cout << std::endl;
  while (reader->read_next()) {
    if (progress != nullptr && progress->wasCanceled()) {
      break;
    }
    ++n_docs_read;
    std::cout << "Read " << n_docs_read << " documents\r" << std::flush;
    n_annotations += insert_doc_record(*(reader->get_current_record()), query);
    if (progress != nullptr) {
      progress->setValue(reader->current_progress());
    }
  }
  std::cout << std::endl;
  query.exec("commit transaction");
  query.exec("select count(*) from document;");
  query.next();
  auto n_after = query.value(0).toInt();
  if (progress != nullptr) {
    progress->setValue(progress->maximum());
  }
  return QList<int>{n_after - n_before, n_annotations};
}

QJsonArray read_json_array(const QString& file_path) {
  QFile file(file_path);
  auto suffix = QFileInfo(file).suffix();
  if (suffix == "json") {
    if (!file.open(QIODevice::ReadOnly)) {
      return QJsonArray{};
    }
    return QJsonDocument::fromJson(file.readAll()).array();
  }
  QJsonArray result{};
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return result;
  }
  QTextStream stream(&file);
  while (!stream.atEnd()) {
    result << QJsonArray{QJsonValue(stream.readLine())};
  }
  return result;
}

int DatabaseCatalog::import_labels(const QString& file_path) {
  QSqlQuery query(QSqlDatabase::database(current_database));
  query.exec("select count(*) from label;");
  query.next();
  auto n_before = query.value(0).toInt();
  auto json_array = read_json_array(file_path);
  query.exec("begin transaction;");
  for (const auto& label_info : json_array) {
    auto label_record = json_to_label_record(label_info);
    insert_label(query, label_record.name, label_record.color,
                 label_record.shortcut_key);
  }
  query.exec("commit transaction;");
  query.exec("select count(*) from label;");
  query.next();
  auto n_after = query.value(0).toInt();
  return n_after - n_before;
}

QList<int> DatabaseCatalog::export_documents(const QString& file_path,
                                             bool labelled_docs_only,
                                             bool include_text,
                                             bool include_annotations,
                                             const QString& user_name,
                                             QProgressDialog* progress) {
  auto suffix = QFileInfo(file_path).suffix();
  std::unique_ptr<DocsWriter> writer{nullptr};
  if (suffix == "xml") {
    writer.reset(new DocsXmlWriter(file_path));
  } else if (suffix == "jsonl") {
    writer.reset(new DocsJsonLinesWriter(file_path));
  } else {
    writer.reset(new DocsJsonWriter(file_path));
  }
  if (!writer->is_open()) {
    return QList<int>{0, 0};
  }

  QSqlQuery query(QSqlDatabase::database(current_database));
  int total_n_docs{};
  if (labelled_docs_only) {
    query.exec("select count(*) from labelled_document;");
    query.next();
    total_n_docs = query.value(0).toInt();
    query.exec("select id from labelled_document order by id;");
  } else {
    query.exec("select count(*) from document;");
    query.next();
    total_n_docs = query.value(0).toInt();
    query.exec("select id from document order by id;");
  }
  if (progress != nullptr) {
    progress->setMaximum(total_n_docs);
  }

  int n_docs{};
  int n_annotations{};
  writer->write_prefix();
  std::cout << std::endl;
  while (query.next()) {
    if (progress != nullptr && progress->wasCanceled()) {
      break;
    }
    ++n_docs;
    auto doc_id = query.value(0).toInt();
    n_annotations += write_doc(*writer, doc_id, include_text,
                               include_annotations, user_name);
    if (progress != nullptr) {
      progress->setValue(n_docs);
    }
    std::cout << "Exported " << n_docs << " documents.\r" << std::flush;
  }
  std::cout << std::endl;
  writer->write_suffix();
  return QList<int>{n_docs, n_annotations};
}

int DatabaseCatalog::write_doc(DocsWriter& writer, int doc_id,
                               bool include_text, bool include_annotations,
                               const QString& user_name) {
  QSqlQuery doc_query(QSqlDatabase::database(current_database));

  if (include_text) {
    doc_query.prepare(
        "select lower(hex(content_md5)) as md5, content, extra_data, title, "
        "short_title, long_title from document where id = :doc;");
  } else {
    doc_query.prepare(
        "select lower(hex(content_md5)) as md5, null as content, extra_data, "
        "title, short_title, long_title from document where id = :doc;");
  }
  doc_query.bindValue(":doc", doc_id);
  doc_query.exec();
  doc_query.next();
  auto extra_data =
      QJsonDocument::fromJson(doc_query.value("extra_data").toByteArray())
          .object();
  int n_annotations{};
  QList<DocsWriter::Annotation> annotations{};
  if (include_annotations) {
    QSqlQuery annotations_query(QSqlDatabase::database(current_database));
    annotations_query.prepare(
        "select label.name as label_name, start_char, end_char from annotation "
        "inner join label on annotation.label_id = label.id "
        "where annotation.doc_id = :doc order by annotation.rowid;");
    annotations_query.bindValue(":doc", doc_id);
    annotations_query.exec();
    while (annotations_query.next()) {
      annotations << DocsWriter::Annotation{
          annotations_query.value("start_char").toInt(),
          annotations_query.value("end_char").toInt(),
          annotations_query.value("label_name").toString()};
      ++n_annotations;
    }
  }
  auto content = doc_query.value("content").isNull()
                     ? QVariant()
                     : doc_query.value("content");
  writer.add_document(doc_query.value("md5").toString(), content, extra_data,
                      include_annotations ? &annotations : nullptr, user_name,
                      doc_query.value("title").toString(),
                      doc_query.value("short_title").toString(),
                      doc_query.value("long_title").toString());
  return n_annotations;
}

int DatabaseCatalog::export_labels(const QString& file_path) {
  QJsonArray labels{};
  QSqlQuery query(QSqlDatabase::database(current_database));
  query.exec("select name, color, shortcut_key from label order by id;");
  while (query.next()) {
    QJsonObject label_info{};
    label_info["text"] = query.value(0).toString();
    label_info["color"] = QColor(query.value(1).toString()).name();
    label_info["background_color"] = QColor(query.value(1).toString()).name();
    auto key = query.value(2);
    if (!key.isNull()) {
      label_info["shortcut_key"] = key.toString();
      label_info["suffix_key"] = key.toString();
    }
    labels << label_info;
  }
  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly)) {
    return 0;
  }
  file.write(QJsonDocument(labels).toJson());
  return labels.size();
}

bool batch_import_export(
    const QString& db_path, const QList<QString>& labels_files,
    const QList<QString>& docs_files, const QString& export_labels_file,
    const QString& export_docs_file, bool labelled_docs_only, bool include_text,
    bool include_annotations, const QString& user_name, bool vacuum) {
  DatabaseCatalog catalog{};
  if (!catalog.open_database(db_path)) {
    std::cerr << "Could not open database: " << db_path.toStdString()
              << std::endl;
    return false;
  }
  if (vacuum) {
    catalog.vacuum_db();
    return true;
  }
  for (const auto& l_file : labels_files) {
    catalog.import_labels(l_file);
  }
  for (const auto& d_file : docs_files) {
    catalog.import_documents(d_file);
  }
  if (export_labels_file != QString()) {
    catalog.export_labels(export_labels_file);
  }
  if (export_docs_file != QString()) {
    catalog.export_documents(export_docs_file, labelled_docs_only, include_text,
                             include_annotations, user_name);
  }
  return true;
}

void DatabaseCatalog::vacuum_db() {
  QSqlQuery query(QSqlDatabase::database(current_database));
  query.exec("VACUUM;");
}

bool DatabaseCatalog::create_tables(QSqlDatabase& database) {
  QSqlQuery query(database);

  query.exec(" CREATE TABLE IF NOT EXISTS document (id INTEGER PRIMARY KEY, "
             "content_md5 BLOB UNIQUE NOT NULL, "
             "content TEXT NOT NULL, extra_data BLOB, title TEXT DEFAULT NULL, "
             "long_title TEXT DEFAULT NULL, short_title TEXT DEFAULT NULL);");

  query.exec("CREATE TABLE IF NOT EXISTS label(id INTEGER PRIMARY KEY, name "
             "TEXT UNIQUE NOT NULL, color TEXT NOT NULL DEFAULT '#FFA000', "
             "shortcut_key TEXT UNIQUE DEFAULT NULL); ");

  query.exec(" CREATE TABLE IF NOT EXISTS annotation(doc_id NOT NULL "
             "REFERENCES document(id) ON DELETE CASCADE, label_id NOT NULL "
             "REFERENCES label(id) ON DELETE CASCADE, start_char INTEGER NOT "
             "NULL, end_char INTEGER NOT NULL, UNIQUE (doc_id, start_char, "
             "end_char, label_id) CHECK (start_char < end_char)); ");

  query.exec(" CREATE INDEX IF NOT EXISTS annotation_doc_id_idx ON "
             "annotation(doc_id);");

  query.exec("CREATE INDEX IF NOT EXISTS annotation_label_id_idx ON "
             "annotation(label_id);");

  query.exec("CREATE TABLE IF NOT EXISTS app_state (last_visited_doc INTEGER "
             "REFERENCES document(id) ON DELETE SET NULL);");

  query.exec("INSERT INTO app_state (last_visited_doc) SELECT (null) WHERE NOT "
             "EXISTS (SELECT * from app_state); ");

  query.exec("CREATE TABLE IF NOT EXISTS app_state_extra (key TEXT UNIQUE NOT "
             "NULL, value); ");

  query.exec(
      "CREATE TABLE IF NOT EXISTS database_info "
      "(database_schema_version INTEGER, created_by_labelbuddy_version TEXT);");

  query.prepare("INSERT INTO database_info "
                "(database_schema_version, "
                "created_by_labelbuddy_version) "
                "SELECT :sv, :lbv "
                "WHERE NOT EXISTS (SELECT * FROM database_info);");
  query.bindValue(":sv", 1);
  query.bindValue(":lbv", get_version());
  query.exec();

  // In experiments with many documents the nested select below seemed much
  // faster than an outer join.
  query.exec("CREATE VIEW IF NOT EXISTS unlabelled_document AS SELECT * FROM "
             "document WHERE id NOT IN (SELECT doc_id FROM annotation); ");

  query.exec("CREATE VIEW IF NOT EXISTS labelled_document AS SELECT distinct * "
             "FROM document WHERE id IN (SELECT doc_id FROM annotation); ");
  if (query.lastError().type() != QSqlError::NoError) {
    return false;
  }
  return true;
}

} // namespace labelbuddy
