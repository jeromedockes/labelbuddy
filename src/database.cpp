#include <cassert>
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

#include "database.h"
#include "utils.h"

namespace labelbuddy {

// about encoding: when reading .txt we assume utf-8 (but qt will detect and
// switch to utf-16 or utf-32 if there is a BOM). when reading or writing json
// (or jsonl) we use utf-8 (json is always utf-8 when exchanged between systems,
// https://tools.ietf.org/html/rfc8259#page-9)

DocsReader::DocsReader(const QString& file_path, QIODevice::OpenMode mode)
    : file_(file_path) {
  if (file_.open(mode)) {
    file_size_ = static_cast<double>(file_.size());
  } else {
    error_code_ = ErrorCode::FileSystemError;
    error_message_ = "Could not open file.";
  }
}

DocsReader::~DocsReader() {}

bool DocsReader::is_open() const { return file_.isOpen(); }

bool DocsReader::has_error() const { return error_code_ != ErrorCode::NoError; }
ErrorCode DocsReader::error_code() const { return error_code_; }
QString DocsReader::error_message() const { return error_message_; }

int DocsReader::progress_max() const { return progress_range_max_; }

int DocsReader::current_progress() const {
  return cast_progress_to_range(static_cast<double>(file_.pos()), file_size_,
                                progress_range_max_);
}

const DocRecord* DocsReader::get_current_record() const {
  return current_record_.get();
}

QFile* DocsReader::get_file() { return &file_; }

void DocsReader::set_current_record(std::unique_ptr<DocRecord> new_record) {
  current_record_ = std::move(new_record);
}

TxtDocsReader::TxtDocsReader(const QString& file_path)
    : DocsReader(file_path), stream_(get_file()) {
  stream_.setCodec("UTF-8");
}

bool TxtDocsReader::read_next() {
  if (!is_open() || stream_.atEnd()) {
    return false;
  }
  std::unique_ptr<DocRecord> new_record(new DocRecord);
  new_record->content = stream_.readLine();
  set_current_record(std::move(new_record));
  return true;
}

std::unique_ptr<DocRecord> json_to_doc_record(const QJsonDocument& json) {
  return json_to_doc_record(json.object());
}

std::unique_ptr<DocRecord> json_to_doc_record(const QJsonValue& json) {
  return json_to_doc_record(json.toObject());
}

std::unique_ptr<DocRecord> json_to_doc_record(const QJsonObject& json) {
  std::unique_ptr<DocRecord> record(new DocRecord);
  if (json.contains("text")) {
    record->content = json["text"].toString();
  } else {
    record->valid_content = false;
  }
  record->declared_md5 = json["utf8_text_md5_checksum"].toString();
  record->annotations = json["labels"].toArray();
  record->metadata =
      QJsonDocument(json["meta"].toObject()).toJson(QJsonDocument::Compact);
  record->short_title = json["short_title"].toString();
  record->long_title = json["long_title"].toString();
  return record;
}

JsonDocsReader::JsonDocsReader(const QString& file_path)
    : DocsReader(file_path) {
  if (has_error()) {
    return;
  }
  QTextStream input_stream(get_file());
  input_stream.setCodec("UTF-8");
  auto json_doc = QJsonDocument::fromJson(input_stream.readAll().toUtf8());
  if (!json_doc.isArray()) {
    total_n_docs_ = 0;
    error_code_ = ErrorCode::CriticalParsingError;
    error_message_ =
        "File does not contain a JSON array.\n(Note: if file is in JSONLines "
        "format, please use the filename extension '.jsonl' rather than "
        "'.json')";
  } else {
    all_docs_ = json_doc.array();
    total_n_docs_ = all_docs_.size();
    current_doc_ = all_docs_.constBegin();
  }
}

bool JsonDocsReader::read_next() {
  if (total_n_docs_ == 0 || current_doc_ == all_docs_.constEnd()) {
    return false;
  }
  set_current_record(json_to_doc_record(*current_doc_));
  ++current_doc_;
  ++n_seen_;
  return true;
}

int JsonDocsReader::progress_max() const { return total_n_docs_; }

int JsonDocsReader::current_progress() const { return n_seen_; }

JsonLinesDocsReader::JsonLinesDocsReader(const QString& file_path)
    : DocsReader(file_path), stream_(get_file()) {
  stream_.setCodec("UTF-8");
}

bool JsonLinesDocsReader::read_next() {
  if (has_error()) {
    return false;
  }
  QString line{};
  while (line == "" && !stream_.atEnd()) {
    line = stream_.readLine();
  }
  if (line == "") {
    return false;
  }
  auto json_doc = QJsonDocument::fromJson(line.toUtf8());
  if (!json_doc.isObject()) {
    error_code_ = ErrorCode::CriticalParsingError;
    error_message_ = "JSONLines error: could not parse line as a JSON object.";
    return false;
  }
  std::unique_ptr<DocRecord> new_record(new DocRecord);
  set_current_record(json_to_doc_record(json_doc));
  return true;
}

DocsWriter::DocsWriter(const QString& file_path, bool include_text,
                       bool include_annotations, bool include_user_name,
                       QIODevice::OpenMode mode)
    : file_(file_path), include_text_{include_text},
      include_annotations_{include_annotations}, include_user_name_{
                                                     include_user_name} {
  file_.open(mode);
}

DocsWriter::~DocsWriter() {}

void DocsWriter::write_prefix() {}
void DocsWriter::write_suffix() {}

bool DocsWriter::is_open() const { return file_.isOpen(); }
bool DocsWriter::is_including_text() const { return include_text_; }
bool DocsWriter::is_including_annotations() const {
  return include_annotations_;
}
bool DocsWriter::is_including_user_name() const { return include_user_name_; }

QFile* DocsWriter::get_file() { return &file_; }

JsonLinesDocsWriter::JsonLinesDocsWriter(const QString& file_path,
                                         bool include_text,
                                         bool include_annotations,
                                         bool include_user_name)
    : DocsWriter(file_path, include_text, include_annotations,
                 include_user_name),
      stream_(get_file()) {
  stream_.setCodec("UTF-8");
}

int JsonLinesDocsWriter::get_n_docs() const { return n_docs_; }

QTextStream& JsonLinesDocsWriter::get_stream() { return stream_; }

void JsonLinesDocsWriter::add_document(
    const QString& md5, const QString& content, const QJsonObject& metadata,
    const QList<Annotation>& annotations, const QString& user_name,
    const QString& short_title, const QString& long_title) {
  if (n_docs_) {
    stream_ << "\n";
  }
  QJsonObject doc_json{};
  assert(md5 != "");
  doc_json.insert("utf8_text_md5_checksum", md5);
  doc_json.insert("meta", metadata);
  if (is_including_user_name() && user_name != QString()) {
    doc_json.insert("annotation_approver", user_name);
  }
  if (is_including_text()) {
    if (short_title != QString()) {
      doc_json.insert("short_title", short_title);
    }
    if (long_title != QString()) {
      doc_json.insert("long_title", long_title);
    }
    assert(content != "");
    doc_json.insert("text", content);
  }
  if (is_including_annotations()) {
    QJsonArray all_annotations_json{};
    for (const auto& annotation : annotations) {
      QJsonArray annotation_json{};
      annotation_json.append(annotation.start_char);
      annotation_json.append(annotation.end_char);
      assert(annotation.label_name != "");
      annotation_json.append(annotation.label_name);
      if (annotation.extra_data != "") {
        annotation_json.append(annotation.extra_data);
      }
      all_annotations_json.append(annotation_json);
    }
    doc_json.insert("labels", all_annotations_json);
  }
  stream_ << QString::fromUtf8(
      QJsonDocument(doc_json).toJson(QJsonDocument::Compact));
  ++n_docs_;
}

void JsonLinesDocsWriter::write_suffix() {
  if (n_docs_) {
    stream_ << "\n";
  }
}

JsonDocsWriter::JsonDocsWriter(const QString& file_path, bool include_text,
                               bool include_annotations, bool include_user_name)
    : JsonLinesDocsWriter(file_path, include_text, include_annotations,
                          include_user_name) {}

void JsonDocsWriter::add_document(const QString& md5, const QString& content,
                                  const QJsonObject& metadata,
                                  const QList<Annotation>& annotations,
                                  const QString& user_name,
                                  const QString& short_title,
                                  const QString& long_title) {
  if (get_n_docs()) {
    get_stream() << ",";
  }
  JsonLinesDocsWriter::add_document(md5, content, metadata, annotations,
                                    user_name, short_title, long_title);
}

void JsonDocsWriter::write_prefix() { get_stream() << "[\n"; }

void JsonDocsWriter::write_suffix() {
  get_stream() << (get_n_docs() ? "\n" : "") << "]\n";
}

LabelRecord json_to_label_record(const QJsonValue& json) {
  LabelRecord record;
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
  return record;
}

const QString DatabaseCatalog::tmp_db_name_{":LABELBUDDY_TEMPORARY_DATABASE:"};

DatabaseCatalog::DatabaseCatalog(QObject* parent) : QObject(parent) {
  open_temp_database(false);
}

QString DatabaseCatalog::get_default_database_path() const {
  QSettings settings("labelbuddy", "labelbuddy");
  if (!settings.contains("last_opened_database")) {
    return QString();
  }
  auto last_opened = settings.value("last_opened_database").toString();
  if (!QFileInfo(last_opened).exists()) {
    return QString();
  }
  return last_opened;
}

RemoveConnection::RemoveConnection(const QString& connection_name)
    : connection_name_{connection_name}, cancelled_{false} {}

RemoveConnection::~RemoveConnection() {
  try {
    execute();
  } catch (...) {
  }
}

void RemoveConnection::execute() const {
  if (!cancelled_ && QSqlDatabase::contains(connection_name_)) {
    QSqlDatabase::removeDatabase(connection_name_);
  }
}

void RemoveConnection::cancel() { cancelled_ = true; }

bool DatabaseCatalog::open_database(const QString& database_path,
                                    bool remember) {
  QString actual_database_path{database_path == QString()
                                   ? get_default_database_path()
                                   : absolute_database_path(database_path)};
  if (actual_database_path == QString()) {
    return false;
  }
  if (QSqlDatabase::contains(actual_database_path)) {
    current_database_ = actual_database_path;
    if (remember)
      store_db_path(actual_database_path);
    return true;
  }
  // Qt's "database name" the param for sqlite call
  // it is the path to db file, or "" if temporary database.
  // the argument to addDatabase is the connection name
  // we choose the db file path for non-temporary and
  // ":LABELBUDDY_TEMPORARY_DATABASE:" otherwise
  auto db_name =
      actual_database_path == tmp_db_name_ ? "" : actual_database_path;
  bool initialized{};
  RemoveConnection remove_con(actual_database_path);
  {
    // scope to make sure db and queries are destroyed before attempting to
    // remove the connection if the initialization fails; see
    // https://doc.qt.io/qt-5/qsqldatabase.html#removeDatabase
    auto db = QSqlDatabase::addDatabase("QSQLITE", actual_database_path);
    db.setDatabaseName(db_name);
    initialized = initialize_database(db);
  }
  if (!initialized) {
    return false;
  }
  remove_con.cancel();
  current_database_ = actual_database_path;
  if (remember)
    store_db_path(actual_database_path);
  emit new_database_opened(actual_database_path);
  return true;
}

bool DatabaseCatalog::is_persistent_database(const QString& db_path) const {
  if (db_path == tmp_db_name_ || db_path == ":memory:" || db_path == "") {
    return false;
  }
  return true;
}

QString
DatabaseCatalog::absolute_database_path(const QString& database_path) const {
  if (is_persistent_database(database_path)) {
    return QFileInfo(database_path).absoluteFilePath();
  }
  return database_path;
}

bool DatabaseCatalog::store_db_path(const QString& db_path) {
  if (!is_persistent_database(db_path)) {
    return false;
  }
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("last_opened_database", db_path);
  return true;
}

QString DatabaseCatalog::open_temp_database(bool load_data) {
  open_database(tmp_db_name_, false);
  if (load_data && (!tmp_db_data_loaded_)) {
    import_labels(":docs/demo_data/example_labels.json");
    import_documents(":docs/demo_data/example_documents.json");
    set_app_state_extra("notebook_page", 0);
    tmp_db_data_loaded_ = true;
    temporary_database_filled(tmp_db_name_);
  }
  return tmp_db_name_;
}

QString DatabaseCatalog::get_current_database() const {
  return current_database_;
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
  QSqlQuery query(QSqlDatabase::database(current_database_));
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
  QSqlQuery query(QSqlDatabase::database(current_database_));
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

QPair<QStringList, QString>
DatabaseCatalog::accepted_and_default_formats(Action action,
                                              ItemKind kind) const {
  switch (action) {
  case Action::Import:
    switch (kind) {
    case ItemKind::Document:
      return {{"txt", "json", "jsonl"}, "txt"};
    case ItemKind::Label:
      return {{"txt", "json", "jsonl"}, "txt"};
    default:
      assert(false);
      return {{}, ""};
    }
  case Action::Export:
    switch (kind) {
    case ItemKind::Document:
      return {{"json", "jsonl"}, "json"};
    case ItemKind::Label:
      return {{"json", "jsonl"}, "json"};
    default:
      assert(false);
      return {{}, ""};
    }
  default:
    assert(false);
    return {{}, ""};
  }
}

QString
DatabaseCatalog::file_extension_error_message(const QString& file_path,
                                              Action action, ItemKind kind,
                                              bool accept_default) const {
  auto valid_and_default = accepted_and_default_formats(action, kind);
  QFileInfo info(file_path);
  auto suffix = info.suffix();
  QString error_msg{};
  if (valid_and_default.first.contains(suffix)) {
    return error_msg;
  }
  QTextStream error_s(&error_msg);
  error_s << (action == Action::Import ? "Import" : "Export") << " "
          << (kind == ItemKind::Document ? "documents" : "labels")
          << ": extension of '" << info.fileName()
          << "' not recognized.\nAccepted formats are: { "
          << valid_and_default.first.join(", ") << " }.";
  if (accept_default) {
    error_s << "\n\nAssuming " << valid_and_default.second << " format.";
  } else {
    error_s
        << "\nPlease rename the file with one of the recognized extensions.";
  }
  return error_msg;
}

int DatabaseCatalog::insert_doc_record(const DocRecord& record,
                                       QSqlQuery& query) {
  QByteArray hash{};
  if (record.valid_content) {
    query.prepare("insert into document (content, content_md5, metadata, "
                  "short_title, long_title) "
                  "values (:content, :md5, :extra, :st, :lt);");
    query.bindValue(":content", record.content);
    query.bindValue(":extra", record.metadata);
    hash = QCryptographicHash::hash(record.content.toUtf8(),
                                    QCryptographicHash::Md5);
    query.bindValue(":md5", hash);
    query.bindValue(":st", record.short_title != QString() ? record.short_title
                                                           : QVariant());
    query.bindValue(":lt", record.long_title != QString() ? record.long_title
                                                          : QVariant());
    query.exec();
  } else {
    if (record.declared_md5 == QString()) {
      return 0;
    }
    // bad chars are skipped. this is not inserted in db but used for lookup.
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
  return insert_doc_annotations(doc_id, record.annotations, query);
}

int DatabaseCatalog::insert_doc_annotations(int doc_id,
                                            const QJsonArray& annotations,
                                            QSqlQuery& query) {
  int n_annotations{};
  for (const auto& annotation : annotations) {
    auto annotation_array = annotation.toArray();
    insert_label(query, annotation_array[2].toString());
    query.prepare("select id from label where name = :lname;");
    query.bindValue(":lname", annotation_array[2].toString());
    query.exec();
    if (!query.next()) {
      // bad annotation (eg empty label)
      continue;
    }
    auto label_id = query.value(0).toInt();
    query.prepare(
        "insert into annotation (doc_id, label_id, start_char, end_char, "
        "extra_data) values (:docid, :labelid, :schar, :echar, :extra);");
    query.bindValue(":docid", doc_id);
    query.bindValue(":labelid", label_id);
    query.bindValue(":schar", annotation_array[0].toInt());
    query.bindValue(":echar", annotation_array[1].toInt());
    if (annotation_array.size() == 4) {
      auto extra = annotation_array[3].toString();
      query.bindValue(":extra", extra != "" ? extra : QVariant());
    } else {
      query.bindValue(":extra", QVariant());
    }
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
    query.prepare("select id from label where shortcut_key = :shortcut;");
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
    query.bindValue(":color", suggest_label_color(color_index_));
    used_default_color = true;
  }
  query.bindValue(":shortcut",
                  (valid_shortcut ? shortcut_key : QVariant(QVariant::String)));
  if (query.exec() && used_default_color) {
    ++color_index_;
  }
}

std::unique_ptr<DocsReader>
DatabaseCatalog::get_docs_reader(const QString& file_path) const {
  std::unique_ptr<DocsReader> reader;
  auto suffix = QFileInfo(file_path).suffix();
  if (suffix == "json") {
    reader.reset(new JsonDocsReader(file_path));
  } else if (suffix == "jsonl") {
    reader.reset(new JsonLinesDocsReader(file_path));
  } else {
    reader.reset(new TxtDocsReader(file_path));
  }
  return reader;
}

ImportDocsResult DatabaseCatalog::import_documents(const QString& file_path,
                                                   QProgressDialog* progress) {
  QSqlQuery query(QSqlDatabase::database(current_database_));
  query.exec("select count(*) from document;");
  query.next();
  auto n_before = query.value(0).toInt();
  auto reader = get_docs_reader(file_path);
  if (reader->has_error()) {
    return {0, 0, reader->error_code(), reader->error_message()};
  }
  if (progress != nullptr) {
    progress->setMaximum(reader->progress_max() + 1);
  }
  bool cancelled{};
  query.exec("begin transaction;");
  int n_annotations{};
  int n_docs_read{};
  std::cout << std::endl;
  while (reader->read_next()) {
    if (progress != nullptr && progress->wasCanceled()) {
      cancelled = true;
      break;
    }
    if (reader->has_error()) {
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
  if (cancelled || reader->has_error()) {
    query.exec("rollback transaction");
  } else {
    query.exec("commit transaction");
  }
  query.exec("select count(*) from document;");
  query.next();
  auto n_after = query.value(0).toInt();
  if (progress != nullptr) {
    progress->setValue(progress->maximum());
  }
  return {n_after - n_before, n_annotations, reader->error_code(),
          reader->error_message()};
}

QPair<QJsonArray, QPair<ErrorCode, QString>>
read_labels(const QString& file_path) {
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly)) {
    return {QJsonArray{}, {ErrorCode::FileSystemError, "Could not open file."}};
  }
  auto suffix = QFileInfo(file).suffix();
  if (suffix == "json") {
    return read_json_labels(file);
  }
  if (suffix == "jsonl") {
    return read_json_lines_labels(file);
  }
  return read_txt_labels(file);
}

QPair<QJsonArray, QPair<ErrorCode, QString>> read_json_labels(QFile& file) {
  auto json_doc = QJsonDocument::fromJson(file.readAll());
  if (!json_doc.isArray()) {
    return {QJsonArray{},
            {ErrorCode::CriticalParsingError,
             "File does not contain a JSON array."}};
  }
  return {json_doc.array(), {ErrorCode::NoError, ""}};
}

QPair<QJsonArray, QPair<ErrorCode, QString>>
read_json_lines_labels(QFile& file) {
  QTextStream stream(&file);
  stream.setCodec("UTF-8");
  QJsonArray result{};
  while (!stream.atEnd()) {
    auto line = stream.readLine();
    if (line == "") {
      continue;
    }
    auto json_doc = QJsonDocument::fromJson(line.toUtf8());
    if (!json_doc.isObject()) {
      return {QJsonArray{},
              {ErrorCode::CriticalParsingError,
               "JSONLines error: could not parse line as a JSON object."}};
    }
    result << json_doc.object();
  }
  return {result, {ErrorCode::NoError, ""}};
}

QPair<QJsonArray, QPair<ErrorCode, QString>> read_txt_labels(QFile& file) {
  QJsonArray result{};
  file.setTextModeEnabled(true);
  QTextStream stream(&file);
  stream.setCodec("UTF-8");
  QJsonObject obj{};
  while (!stream.atEnd()) {
    obj["text"] = stream.readLine();
    result << obj;
  }
  return {result, {ErrorCode::NoError, ""}};
}

ImportLabelsResult DatabaseCatalog::import_labels(const QString& file_path) {
  QSqlQuery query(QSqlDatabase::database(current_database_));
  query.exec("select count(*) from label;");
  query.next();
  auto n_before = query.value(0).toInt();
  auto read_result = read_labels(file_path);
  if (read_result.second.first != ErrorCode::NoError) {
    return {0, read_result.second.first, read_result.second.second};
  }
  query.exec("begin transaction;");
  for (const auto& label_info : read_result.first) {
    auto label_record = json_to_label_record(label_info);
    insert_label(query, label_record.name, label_record.color,
                 label_record.shortcut_key);
  }
  query.exec("commit transaction;");
  query.exec("select count(*) from label;");
  query.next();
  auto n_after = query.value(0).toInt();
  return {n_after - n_before, ErrorCode::NoError, ""};
}

std::unique_ptr<DocsWriter>
DatabaseCatalog::get_docs_writer(const QString& file_path, bool include_text,
                                 bool include_annotations,
                                 bool include_user_name) const {

  auto suffix = QFileInfo(file_path).suffix();
  std::unique_ptr<DocsWriter> writer{nullptr};
  if (suffix == "jsonl") {
    writer.reset(new JsonLinesDocsWriter(
        file_path, include_text, include_annotations, include_user_name));
  } else {
    writer.reset(new JsonDocsWriter(file_path, include_text,
                                    include_annotations, include_user_name));
  }
  return writer;
}

ExportDocsResult DatabaseCatalog::export_documents(const QString& file_path,
                                                   bool labelled_docs_only,
                                                   bool include_text,
                                                   bool include_annotations,
                                                   const QString& user_name,
                                                   QProgressDialog* progress) {
  bool include_user_name = user_name != "";
  auto writer = get_docs_writer(file_path, include_text, include_annotations,
                                include_user_name);
  if (!writer->is_open()) {
    return {0, 0, ErrorCode::FileSystemError, QString("Could not open file.")};
  }

  QSqlQuery query(QSqlDatabase::database(current_database_));
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
    progress->setMaximum(total_n_docs + 1);
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
  if (progress != nullptr) {
    progress->setValue(progress->maximum());
  }
  return {n_docs, n_annotations, ErrorCode::NoError, ""};
}

int DatabaseCatalog::write_doc(DocsWriter& writer, int doc_id,
                               bool include_text, bool include_annotations,
                               const QString& user_name) {
  QSqlQuery doc_query(QSqlDatabase::database(current_database_));

  if (include_text) {
    doc_query.prepare("select lower(hex(content_md5)) as md5, content, "
                      "metadata, "
                      "short_title, long_title from document where id = :doc;");
  } else {
    doc_query.prepare(
        "select lower(hex(content_md5)) as md5, null as content, metadata, "
        "null as short_title, null as long_title "
        "from document where id = :doc;");
  }
  doc_query.bindValue(":doc", doc_id);
  doc_query.exec();
  doc_query.next();
  auto metadata =
      QJsonDocument::fromJson(doc_query.value("metadata").toByteArray())
          .object();
  int n_annotations{};
  QList<DocsWriter::Annotation> annotations{};
  if (include_annotations) {
    QSqlQuery annotations_query(QSqlDatabase::database(current_database_));
    annotations_query.prepare(
        "select label.name as label_name, start_char, end_char, extra_data "
        "from annotation inner join label on annotation.label_id = label.id "
        "where annotation.doc_id = :doc order by annotation.rowid;");
    annotations_query.bindValue(":doc", doc_id);
    annotations_query.exec();
    while (annotations_query.next()) {
      annotations << DocsWriter::Annotation{
          annotations_query.value("start_char").toInt(),
          annotations_query.value("end_char").toInt(),
          annotations_query.value("label_name").toString(),
          annotations_query.value("extra_data").toString()};
      ++n_annotations;
    }
  }
  writer.add_document(doc_query.value("md5").toString(),
                      doc_query.value("content").toString(), metadata,
                      annotations, user_name,
                      doc_query.value("short_title").toString(),
                      doc_query.value("long_title").toString());
  return n_annotations;
}

ExportLabelsResult DatabaseCatalog::export_labels(const QString& file_path) {
  QJsonArray labels{};
  QSqlQuery query(QSqlDatabase::database(current_database_));
  query.exec("select name, color, shortcut_key from sorted_label;");
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
  auto suffix = QFileInfo(file_path).suffix();
  if (suffix == "jsonl") {
    return write_labels_to_json_lines(labels, file_path);
  }
  return write_labels_to_json(labels, file_path);
}

ExportLabelsResult
DatabaseCatalog::write_labels_to_json(const QJsonArray& labels,
                                      const QString& file_path) {
  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly)) {
    return {0, ErrorCode::FileSystemError, QString("Could not open file.")};
  }
  file.write(QJsonDocument(labels).toJson());

  return {labels.size(), ErrorCode::NoError, ""};
}

ExportLabelsResult
DatabaseCatalog::write_labels_to_json_lines(const QJsonArray& labels,
                                            const QString& file_path) {
  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly)) {
    return {0, ErrorCode::FileSystemError, QString("Could not open file.")};
  }
  QTextStream stream(&file);
  stream.setCodec("UTF-8");
  for (const auto& label_info : labels) {
    stream
        << QJsonDocument(label_info.toObject()).toJson(QJsonDocument::Compact)
        << "\n";
  }
  return {labels.size(), ErrorCode::NoError, ""};
}

int batch_import_export(
    const QString& db_path, const QList<QString>& labels_files,
    const QList<QString>& docs_files, const QString& export_labels_file,
    const QString& export_docs_file, bool labelled_docs_only, bool include_text,
    bool include_annotations, const QString& user_name, bool vacuum) {
  DatabaseCatalog catalog{};
  if (!catalog.open_database(db_path, false)) {
    std::cerr << "Could not open database: " << db_path.toStdString()
              << std::endl;
    return 1;
  }
  if (vacuum) {
    catalog.vacuum_db();
    return 0;
  }
  int errors{};
  QString error_msg{};
  for (const auto& l_file : labels_files) {
    error_msg = catalog.file_extension_error_message(
        l_file, DatabaseCatalog::Action::Import,
        DatabaseCatalog::ItemKind::Label, false);
    if (error_msg == QString()) {
      auto res = catalog.import_labels(l_file);
      if (res.error_code != ErrorCode::NoError) {
        errors = 1;
      }
    } else {
      errors = 1;
      std::cerr << error_msg.toStdString() << std::endl;
    }
  }
  for (const auto& d_file : docs_files) {
    error_msg = catalog.file_extension_error_message(
        d_file, DatabaseCatalog::Action::Import,
        DatabaseCatalog::ItemKind::Document, false);
    if (error_msg == QString()) {
      auto res = catalog.import_documents(d_file);
      if (res.error_code != ErrorCode::NoError) {
        errors = 1;
      }
    } else {
      errors = 1;
      std::cerr << error_msg.toStdString() << std::endl;
    }
  }
  if (export_labels_file != QString()) {
    error_msg = catalog.file_extension_error_message(
        export_labels_file, DatabaseCatalog::Action::Export,
        DatabaseCatalog::ItemKind::Label, true);
    if (error_msg != QString()) {
      std::cerr << error_msg.toStdString() << std::endl;
      // still exported, so don't count it as an error
    }
    auto res = catalog.export_labels(export_labels_file);
    if (res.error_code != ErrorCode::NoError) {
      errors = 1;
    }
  }
  if (export_docs_file != QString()) {
    error_msg = catalog.file_extension_error_message(
        export_docs_file, DatabaseCatalog::Action::Export,
        DatabaseCatalog::ItemKind::Document, true);
    if (error_msg != QString()) {
      std::cerr << error_msg.toStdString() << std::endl;
      // still exported, so don't count it as an error
    }
    auto res =
        catalog.export_documents(export_docs_file, labelled_docs_only,
                                 include_text, include_annotations, user_name);
    if (res.error_code != ErrorCode::NoError) {
      errors = 1;
    }
  }
  return errors;
}

void DatabaseCatalog::vacuum_db() {
  QSqlQuery query(QSqlDatabase::database(current_database_));
  query.exec("VACUUM;");
}

bool DatabaseCatalog::initialize_database(QSqlDatabase& database) {
  if (!database.open()) {
    return false;
  }
  QSqlQuery query(database);
  // file not empty and is not an SQLite db
  if (!query.exec("PRAGMA schema_version")) {
    return false;
  }
  query.next();
  auto sqlite_schema_version = query.value(0).toInt();

  // db existed before (schema has been modified if schema version != 0)
  if (sqlite_schema_version != 0) {
    query.exec("PRAGMA application_id;");
    query.next();
    // not a labelbuddy db
    if (query.value(0).toInt() != sqlite_application_id_) {
      return false;
    }
    query.exec("PRAGMA user_version;");
    query.next();
    if (query.value(0).toInt() != sqlite_user_version_) {
      return false;
    }
    // already contains a labelbuddy db
    // check it is not readonly
    if (!query.exec(QString("PRAGMA application_id = %1;")
                        .arg(sqlite_application_id_))) {
      return false;
    }
    return (query.exec("PRAGMA foreign_keys = ON;"));
  }
  if (!query.exec("PRAGMA foreign_keys = ON;")) {
    return false;
  }
  return create_tables(query);
}

bool DatabaseCatalog::create_tables(QSqlQuery& query) {
  query.exec("BEGIN TRANSACTION;");
  bool success{true};
  success *= query.exec(
      QString("PRAGMA application_id = %1;").arg(sqlite_application_id_));
  success *= query.exec(
      QString("PRAGMA user_version = %1;").arg(sqlite_user_version_));

  success *=
      query.exec("CREATE TABLE IF NOT EXISTS document (id INTEGER PRIMARY KEY, "
                 "content_md5 BLOB UNIQUE NOT NULL, "
                 "content TEXT NOT NULL, metadata BLOB, "
                 "long_title TEXT DEFAULT NULL, short_title TEXT DEFAULT NULL, "
                 "CHECK (content != ''), CHECK (length(content_md5 = 128)));");
  // for some reason the auto index created for the primary key is not treated
  // as a covering index in 'count(*) from document where id < xxx' but this is:
  success *= query.exec("CREATE INDEX IF NOT EXISTS document_id_idx ON "
                        "document(id);");

  success *= query.exec(
      "CREATE TABLE IF NOT EXISTS label(id INTEGER PRIMARY KEY, name "
      "TEXT UNIQUE NOT NULL, color TEXT NOT NULL DEFAULT '#FFA000', "
      "shortcut_key TEXT UNIQUE DEFAULT NULL, "
      "display_order INTEGER DEFAULT NULL, CHECK (name != '')); ");
  // NULLS LAST only available from sqlite 3.30
  success *= query.exec(
      "CREATE VIEW IF NOT EXISTS sorted_label AS SELECT * FROM label ORDER BY "
      "display_order IS NULL, display_order, id;");

  success *= query.exec(
      " CREATE TABLE IF NOT EXISTS annotation(doc_id NOT NULL "
      "REFERENCES document(id) ON DELETE CASCADE, label_id NOT NULL "
      "REFERENCES label(id) ON DELETE CASCADE, start_char INTEGER NOT "
      "NULL, end_char INTEGER NOT NULL, extra_data TEXT DEFAULT NULL, "
      "UNIQUE (doc_id, start_char, end_char, label_id) "
      "CHECK (start_char < end_char)); ");

  success *= query.exec(" CREATE INDEX IF NOT EXISTS annotation_doc_id_idx ON "
                        "annotation(doc_id);");

  success *= query.exec("CREATE INDEX IF NOT EXISTS annotation_label_id_idx ON "
                        "annotation(label_id);");

  success *= query.exec(
      "CREATE TABLE IF NOT EXISTS app_state (last_visited_doc INTEGER "
      "REFERENCES document(id) ON DELETE SET NULL);");

  success *= query.exec(
      "INSERT INTO app_state (last_visited_doc) SELECT (null) WHERE NOT "
      "EXISTS (SELECT * from app_state); ");

  success *= query.exec(
      "CREATE TABLE IF NOT EXISTS app_state_extra (key TEXT UNIQUE NOT "
      "NULL, value); ");

  success *= query.exec(
      "CREATE TABLE IF NOT EXISTS database_info "
      "(database_schema_version INTEGER, created_by_labelbuddy_version TEXT);");

  query.prepare("INSERT INTO database_info "
                "(database_schema_version, "
                "created_by_labelbuddy_version) "
                "SELECT :uv, :lbv "
                "WHERE NOT EXISTS (SELECT * FROM database_info);");
  query.bindValue(":uv", sqlite_user_version_);
  query.bindValue(":lbv", get_version());
  success *= query.exec();

  // In experiments with many documents the subqueries below seemed much
  // faster than a left join and slightly faster than using 'where not exists'.
  success *= query.exec(
      "CREATE VIEW IF NOT EXISTS unlabelled_document AS SELECT * FROM "
      "document WHERE id NOT IN (SELECT distinct doc_id FROM annotation); ");

  success *= query.exec(
      "CREATE VIEW IF NOT EXISTS labelled_document AS SELECT * "
      "FROM document WHERE id IN (SELECT distinct doc_id FROM annotation); ");
  if (success) {
    query.exec("COMMIT;");
    return true;
  }
  query.exec("ROLLBACK;");
  return false;
}

} // namespace labelbuddy
