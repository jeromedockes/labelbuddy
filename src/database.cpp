#include <memory>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProgressDialog>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QString>
#include <QXmlStreamWriter>

#include "database.h"

namespace labelbuddy {

// about encoding: when reading .txt we use the Qt default (depends on locale)
// when reading or writing json (or jsonl) we force utf-8 (json is always utf-8)
// when reading xml the encoding should specified in the xml prolog
// when writing xml we force utf-8 (which is the Qt default on all platforms)

DocsReader::DocsReader(const QString& file_path) : file(file_path) {
  file.open(QIODevice::ReadOnly | QIODevice::Text);
}

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
  std::unique_ptr<DocRecord> new_record(new DocRecord{stream.readLine(), "{}"});
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
  QJsonObject json{};
  for (auto attrib : xml.attributes()) {
    json[attrib.name().toString()] = attrib.value().toString();
  }
  std::unique_ptr<DocRecord> new_record(new DocRecord);
  new_record->extra_data = QJsonDocument(json).toJson(QJsonDocument::Compact);
  new_record->content =
      xml.readElementText(QXmlStreamReader::IncludeChildElements);
  set_current_record(std::move(new_record));
  return true;
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
  record->content = json["text"].toString();
  record->extra_data =
      QJsonDocument(json["meta"].toObject()).toJson(QJsonDocument::Compact);
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

AnnotationsWriter::AnnotationsWriter(const QString& file_path)
    : file(file_path) {
  file.open(QIODevice::WriteOnly | QIODevice::Text);
}

void AnnotationsWriter::write_prefix() {}
void AnnotationsWriter::write_suffix() {}

bool AnnotationsWriter::is_open() const { return file.isOpen(); }

QFile* AnnotationsWriter::get_file() { return &file; }

void AnnotationsWriter::add_document(const QString& md5,
                                     const QVariant& content,
                                     const QJsonObject& extra_data,
                                     const QList<Annotation> annotations,
                                     const QString& user_name) {
  (void)md5;
  (void)content;
  (void)extra_data;
  (void)annotations;
  (void)user_name;
}

AnnotationsJsonLinesWriter::AnnotationsJsonLinesWriter(const QString& file_path)
    : AnnotationsWriter(file_path), stream(get_file()) {
  stream.setCodec("UTF-8");
}

int AnnotationsJsonLinesWriter::get_n_docs() const { return n_docs; }

QTextStream& AnnotationsJsonLinesWriter::get_stream() { return stream; }

void AnnotationsJsonLinesWriter::add_document(
    const QString& md5, const QVariant& content, const QJsonObject& extra_data,
    const QList<Annotation> annotations, const QString& user_name) {
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
  QJsonArray all_annotations_json{};
  for (auto annotation : annotations) {
    QJsonArray annotation_json{};
    annotation_json.append(annotation.start_char);
    annotation_json.append(annotation.end_char);
    annotation_json.append(annotation.label_name);
    all_annotations_json.append(annotation_json);
  }
  doc_json.insert("labels", all_annotations_json);

  stream << QString::fromUtf8(
      QJsonDocument(doc_json).toJson(QJsonDocument::Compact));
  ++n_docs;
}

void AnnotationsJsonLinesWriter::write_suffix() {
  if (n_docs) {
    stream << "\n";
  }
}

AnnotationsJsonWriter::AnnotationsJsonWriter(const QString& file_path)
    : AnnotationsJsonLinesWriter(file_path) {}

void AnnotationsJsonWriter::add_document(const QString& md5,
                                         const QVariant& content,
                                         const QJsonObject& extra_data,
                                         const QList<Annotation> annotations,
                                         const QString& user_name) {
  if (get_n_docs()) {
    get_stream() << ",";
  }
  AnnotationsJsonLinesWriter::add_document(md5, content, extra_data,
                                           annotations, user_name);
}

void AnnotationsJsonWriter::write_prefix() { get_stream() << "[\n"; }

void AnnotationsJsonWriter::write_suffix() {
  get_stream() << (get_n_docs() ? "\n" : "") << "]\n";
}

AnnotationsXmlWriter::AnnotationsXmlWriter(const QString& file_path)
    : AnnotationsWriter(file_path), xml(get_file()) {
  xml.setCodec("UTF-8");
  xml.setAutoFormatting(true);
}

void AnnotationsXmlWriter::write_prefix() {
  xml.writeStartDocument();
  xml.writeStartElement("annotated_document_set");
}

void AnnotationsXmlWriter::write_suffix() {
  xml.writeEndElement();
  xml.writeEndDocument();
}

void AnnotationsXmlWriter::add_document(const QString& md5,
                                        const QVariant& content,
                                        const QJsonObject& extra_data,
                                        const QList<Annotation> annotations,
                                        const QString& user_name) {

  xml.writeStartElement("annotated_document");
  xml.writeTextElement("document_md5_checksum", md5);
  xml.writeStartElement(content != QVariant() ? "document" : "meta");
  for (auto key_val = extra_data.constBegin(); key_val != extra_data.constEnd();
       ++key_val) {
    xml.writeAttribute(key_val.key(), key_val.value().toVariant().toString());
  }
  if (content != QVariant()) {
    xml.writeCharacters(content.toString());
  }
  xml.writeEndElement();
  if (user_name != QString()) {
    xml.writeTextElement("annotation_approver", user_name);
  }
  add_annotations(annotations);
  xml.writeEndElement();
}

void AnnotationsXmlWriter::add_annotations(
    const QList<Annotation> annotations) {

  xml.writeStartElement("annotation_set");
  for (auto annotation : annotations) {
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
    record.name = json.toObject()["text"].toString();
    record.color = json.toObject()["background_color"].toString();
  } else {
    record.name = json.toArray()[0].toString();
    record.color = json.toArray()[1].toString();
  }
  return record;
}

QString DatabaseCatalog::get_default_database_path() {
  auto home_dir =
      QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0];
  auto default_path = QDir(home_dir).filePath("labelbuddy_data.sqlite3");
  QSettings settings("labelbuddy", "labelbuddy");
  return settings.value("last_opened_database", default_path).toString();
}

void DatabaseCatalog::open_database(const QString& database_path) {
  QString actual_database_path = (database_path == QString())
                                     ? get_default_database_path()
                                     : database_path;
  if (databases.contains(actual_database_path)) {
    current_database = actual_database_path;
    return;
  }
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", actual_database_path);
  db.setDatabaseName(actual_database_path);
  db.open();
  QSqlQuery query(db);
  query.exec("PRAGMA foreign_keys = ON;");
  current_database = actual_database_path;
  databases.insert(actual_database_path);
  create_tables();
}

QString DatabaseCatalog::get_current_database() const {
  return current_database;
}

QString DatabaseCatalog::parent_directory(const QString& file_path) const {
  auto dir = QDir(file_path);
  dir.cdUp();
  return dir.absolutePath();
}

QString DatabaseCatalog::get_current_directory() const {
  return parent_directory(current_database);
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

int DatabaseCatalog::import_documents(const QString& file_path,
                                      QWidget* parent) {
  QSqlQuery query(QSqlDatabase::database(current_database));
  query.exec("select count(*) from document;");
  query.next();
  auto n_before = query.value(0).toInt();

  QProgressDialog progress("Importing documents...", "Stop", 0, 0, parent);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(2000);

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

  progress.setMaximum(reader->progress_max());
  while (reader->read_next()) {
    if (progress.wasCanceled()) {
      break;
    }

    query.prepare("insert into document (content, content_md5, extra_data) "
                  "values (:content, :md5, :extra)");
    auto current_record = reader->get_current_record();
    query.bindValue(":content", current_record->content);
    query.bindValue(":extra", current_record->extra_data);
    auto hash = QCryptographicHash::hash(current_record->content.toUtf8(),
                                         QCryptographicHash::Md5);
    query.bindValue(":md5", hash);
    query.exec();
    progress.setValue(reader->current_progress());
  }

  query.exec("select count(*) from document;");
  query.next();
  auto n_after = query.value(0).toInt();
  return n_after - n_before;
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
  QStringList colors{"#aec7e8", "#ffbb78", "#98df8a", "#ff9896", "#c5b0d5",
                     "#c49c94", "#f7b6d2", "#c7c7c7", "#dbdb8d", "#9edae5"};
  int color_index{};
  for (auto label_info : json_array) {
    auto label_record = json_to_label_record(label_info);
    if (!QColor::isValidColor(label_record.color)) {
      label_record.color = colors[color_index % colors.length()];
      ++color_index;
    }
    query.prepare("insert into label (name, color) values (:name, :color)");
    query.bindValue(":name", label_record.name);
    query.bindValue(":color", label_record.color);
    query.exec();
  }
  query.exec("select count(*) from label;");
  query.next();
  auto n_after = query.value(0).toInt();
  return n_after - n_before;
}

QList<int> DatabaseCatalog::export_annotations(const QString& file_path,
                                               bool labelled_docs_only,
                                               bool include_documents,
                                               const QString& user_name) {

  auto suffix = QFileInfo(file_path).suffix();
  std::unique_ptr<AnnotationsWriter> writer;
  if (suffix == "json") {
    writer.reset(new AnnotationsJsonWriter(file_path));
  } else if (suffix == "jsonl") {
    writer.reset(new AnnotationsJsonLinesWriter(file_path));
  } else {
    writer.reset(new AnnotationsXmlWriter(file_path));
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
  QProgressDialog progress("Exporting annotations...", "Stop", 0, total_n_docs,
                           this);
  progress.setWindowModality(Qt::WindowModal);

  int n_docs{};
  int n_annotations{};
  writer->write_prefix();
  while (query.next()) {
    progress.setValue(n_docs);
    if (progress.wasCanceled()) {
      break;
    }
    ++n_docs;
    auto doc_id = query.value(0).toInt();
    n_annotations += write_doc_and_annotations(*writer, doc_id,
                                               include_documents, user_name);
  }
  writer->write_suffix();
  return QList<int>{n_docs, n_annotations};
}

int DatabaseCatalog::write_doc_and_annotations(AnnotationsWriter& writer,
                                               int doc_id,
                                               bool include_document,
                                               const QString& user_name) {
  QSqlQuery doc_query(QSqlDatabase::database(current_database));
  QSqlQuery annotations_query(QSqlDatabase::database(current_database));

  if (include_document) {
    doc_query.prepare("select lower(hex(content_md5)) as md5, content, "
                      "extra_data from document where id = :doc;");
  } else {
    doc_query.prepare("select lower(hex(content_md5)) as md5, null as content, "
                      "extra_data from document where id = :doc;");
  }
  doc_query.bindValue(":doc", doc_id);
  doc_query.exec();
  doc_query.next();
  auto extra_data =
      QJsonDocument::fromJson(doc_query.value("extra_data").toByteArray())
          .object();
  annotations_query.prepare(
      "select label.name as label_name, start_char, end_char from annotation "
      "inner join label on annotation.label_id = label.id "
      "where annotation.doc_id = :doc order by annotation.rowid;");
  annotations_query.bindValue(":doc", doc_id);
  annotations_query.exec();
  QList<AnnotationsWriter::Annotation> annotations{};
  int n_annotations{};
  while (annotations_query.next()) {
    annotations << AnnotationsWriter::Annotation{
        annotations_query.value("start_char").toInt(),
        annotations_query.value("end_char").toInt(),
        annotations_query.value("label_name").toString()};
    ++n_annotations;
  }

  auto content = doc_query.value("content").isNull()
                     ? QVariant()
                     : doc_query.value("content");
  writer.add_document(doc_query.value("md5").toString(), content, extra_data,
                      annotations, user_name);
  return n_annotations;
}

void DatabaseCatalog::create_tables() {
  QSqlQuery query(QSqlDatabase::database(current_database));

  query.exec(" CREATE TABLE IF NOT EXISTS document (id INTEGER PRIMARY KEY, "
             "content_md5 BLOB UNIQUE NOT NULL, "
             "content TEXT NOT NULL, extra_data BLOB); ");

  query.exec("CREATE TABLE IF NOT EXISTS label(id INTEGER PRIMARY KEY, name "
             "TEXT UNIQUE NOT NULL, color TEXT NOT NULL DEFAULT '#FFA000'); ");

  query.exec(" CREATE TABLE IF NOT EXISTS annotation(doc_id NOT NULL "
             "REFERENCES document(id) ON DELETE CASCADE, label_id NOT NULL "
             "REFERENCES label(id) ON DELETE CASCADE, start_char INTEGER NOT "
             "NULL, end_char INTEGER NOT NULL); ");

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

  query.exec("CREATE VIEW IF NOT EXISTS unlabelled_document AS SELECT * FROM "
             "document WHERE id NOT IN (SELECT doc_id FROM annotation); ");

  query.exec("CREATE VIEW IF NOT EXISTS labelled_document AS SELECT distinct * "
             "FROM document WHERE id IN (SELECT doc_id FROM annotation); ");
}

} // namespace labelbuddy
