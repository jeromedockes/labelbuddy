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
#include "database_impl.h"
#include "utils.h"

namespace labelbuddy {

// about encoding: when reading .txt we assume utf-8 (but qt will detect and
// switch to utf-16 or utf-32 if there is a BOM). when reading or writing json
// (or jsonl) we use utf-8 (json is always utf-8 when exchanged between systems,
// https://tools.ietf.org/html/rfc8259#page-9)

constexpr int Annotation::nullIndex;

DocsReader::DocsReader(const QString& filePath) : file_(filePath) {
  if (file_.open(QIODevice::ReadOnly | QIODevice::Text)) {
    fileSize_ = static_cast<double>(file_.size());
  } else {
    setError(ErrorCode::FileSystemError, "Could not open file.");
  }
}

bool DocsReader::isOpen() const { return file_.isOpen(); }

bool DocsReader::hasError() const { return errorCode_ != ErrorCode::NoError; }

ErrorCode DocsReader::errorCode() const { return errorCode_; }

QString DocsReader::errorMessage() const { return errorMessage_; }

int DocsReader::progressMax() const { return progressRangeMax_; }

int DocsReader::currentProgress() const {
  return castProgressToRange(static_cast<double>(file_.pos()), fileSize_,
                             progressRangeMax_);
}

const DocRecord* DocsReader::getCurrentRecord() const {
  return currentRecord_.get();
}

QFile* DocsReader::getFile() { return &file_; }

void DocsReader::setCurrentRecord(std::unique_ptr<DocRecord> newRecord) {
  currentRecord_ = std::move(newRecord);
}

void DocsReader::setError(ErrorCode code, const QString& message) {
  errorCode_ = code;
  errorMessage_ = message;
}

TxtDocsReader::TxtDocsReader(const QString& filePath)
    : DocsReader(filePath), stream_(getFile()) {
  stream_.setCodec("UTF-8");
}

bool TxtDocsReader::readNext() {
  if (!isOpen() || stream_.atEnd()) {
    return false;
  }
  std::unique_ptr<DocRecord> newRecord(new DocRecord);
  newRecord->content = stream_.readLine();
  setCurrentRecord(std::move(newRecord));
  return true;
}

std::unique_ptr<DocRecord> jsonToDocRecord(const QJsonDocument& json) {
  return jsonToDocRecord(json.object());
}

std::unique_ptr<DocRecord> jsonToDocRecord(const QJsonValue& json) {
  return jsonToDocRecord(json.toObject());
}

std::unique_ptr<DocRecord> jsonToDocRecord(const QJsonObject& json) {
  std::unique_ptr<DocRecord> record(new DocRecord);
  if (json.contains("text")) {
    record->content = json["text"].toString();
  } else {
    record->validContent = false;
  }
  record->declaredMd5 = json["utf8_text_md5_checksum"].toString();
  for (const auto& annotation : json["annotations"].toArray()) {
    auto annotationObject = annotation.toObject();
    record->annotations << Annotation{
        annotationObject["start_char"].toInt(Annotation::nullIndex),
        annotationObject["end_char"].toInt(Annotation::nullIndex),
        annotationObject["label_name"].toString(),
        annotationObject["extra_data"].toString(""),
        annotationObject["start_byte"].toInt(Annotation::nullIndex),
        annotationObject["end_byte"].toInt(Annotation::nullIndex),
    };
  }
  record->metadata =
      QJsonDocument(json["metadata"].toObject()).toJson(QJsonDocument::Compact);
  record->displayTitle = json["display_title"].toString();
  record->listTitle = json["list_title"].toString();
  return record;
}

JsonDocsReader::JsonDocsReader(const QString& filePath) : DocsReader(filePath) {
  if (hasError()) {
    return;
  }
  QTextStream inputStream(getFile());
  inputStream.setCodec("UTF-8");
  auto jsonDoc = QJsonDocument::fromJson(inputStream.readAll().toUtf8());
  if (!jsonDoc.isArray()) {
    totalNDocs_ = 0;
    setError(
        ErrorCode::CriticalParsingError,
        "File does not contain a JSON array.\n(Note: if file is in JSONLines "
        "format, please use the filename extension '.jsonl' rather than "
        "'.json')");
  } else {
    allDocs_ = jsonDoc.array();
    totalNDocs_ = allDocs_.size();
    currentDoc_ = allDocs_.constBegin();
  }
}

bool JsonDocsReader::readNext() {
  if (totalNDocs_ == 0 || currentDoc_ == allDocs_.constEnd()) {
    return false;
  }
  setCurrentRecord(jsonToDocRecord(*currentDoc_));
  ++currentDoc_;
  ++nSeen_;
  return true;
}

int JsonDocsReader::progressMax() const { return totalNDocs_; }

int JsonDocsReader::currentProgress() const { return nSeen_; }

JsonLinesDocsReader::JsonLinesDocsReader(const QString& filePath)
    : DocsReader(filePath), stream_(getFile()) {
  stream_.setCodec("UTF-8");
}

bool JsonLinesDocsReader::readNext() {
  if (hasError()) {
    return false;
  }
  QString line{};
  while (line == "" && !stream_.atEnd()) {
    line = stream_.readLine();
  }
  if (line == "") {
    return false;
  }
  auto jsonDoc = QJsonDocument::fromJson(line.toUtf8());
  if (!jsonDoc.isObject()) {
    setError(ErrorCode::CriticalParsingError,
             "JSONLines error: could not parse line as a JSON object.");
    return false;
  }
  std::unique_ptr<DocRecord> newRecord(new DocRecord);
  setCurrentRecord(jsonToDocRecord(jsonDoc));
  return true;
}

DocsWriter::DocsWriter(const QString& filePath, bool includeText,
                       bool includeAnnotations)
    : file_(filePath), includeText_{includeText}, includeAnnotations_{
                                                      includeAnnotations} {
  file_.open(QIODevice::WriteOnly | QIODevice::Text);
}

void DocsWriter::writePrefix() {}

void DocsWriter::writeSuffix() {}

bool DocsWriter::isOpen() const { return file_.isOpen(); }

bool DocsWriter::isIncludingText() const { return includeText_; }

bool DocsWriter::isIncludingAnnotations() const { return includeAnnotations_; }

QFile* DocsWriter::getFile() { return &file_; }

JsonLinesDocsWriter::JsonLinesDocsWriter(const QString& filePath,
                                         bool includeText,
                                         bool includeAnnotations)
    : DocsWriter(filePath, includeText, includeAnnotations),
      stream_(getFile()) {
  stream_.setCodec("UTF-8");
}

int JsonLinesDocsWriter::getNDocs() const { return nDocs_; }

QTextStream& JsonLinesDocsWriter::getStream() { return stream_; }

void JsonLinesDocsWriter::addDocument(const QString& md5,
                                      const QString& content,
                                      const QJsonObject& metadata,
                                      const QList<Annotation>& annotations,
                                      const QString& displayTitle,
                                      const QString& listTitle) {
  if (nDocs_) {
    stream_ << "\n";
  }
  QJsonObject docJson{};
  assert(md5 != "");
  docJson.insert("utf8_text_md5_checksum", md5);
  docJson.insert("metadata", metadata);
  if (isIncludingText()) {
    if (displayTitle != QString()) {
      docJson.insert("display_title", displayTitle);
    }
    if (listTitle != QString()) {
      docJson.insert("list_title", listTitle);
    }
    assert(content != "");
    docJson.insert("text", content);
  }
  if (isIncludingAnnotations()) {
    QJsonArray allAnnotationsJson{};
    for (const auto& annotation : annotations) {
      QJsonObject annotationJson{};
      annotationJson.insert("start_char", annotation.startChar);
      annotationJson.insert("end_char", annotation.endChar);
      annotationJson.insert("start_byte", annotation.startByte);
      annotationJson.insert("end_byte", annotation.endByte);
      assert(annotation.labelName != "");
      annotationJson.insert("label_name", annotation.labelName);
      if (annotation.extraData != "") {
        annotationJson.insert("extra_data", annotation.extraData);
      }
      allAnnotationsJson.append(annotationJson);
    }
    docJson.insert("annotations", allAnnotationsJson);
  }
  stream_ << QString::fromUtf8(
      QJsonDocument(docJson).toJson(QJsonDocument::Compact));
  ++nDocs_;
}

void JsonLinesDocsWriter::writeSuffix() {
  if (nDocs_) {
    stream_ << "\n";
  }
}

JsonDocsWriter::JsonDocsWriter(const QString& filePath, bool includeText,
                               bool includeAnnotations)
    : JsonLinesDocsWriter(filePath, includeText, includeAnnotations) {}

void JsonDocsWriter::addDocument(const QString& md5, const QString& content,
                                 const QJsonObject& metadata,
                                 const QList<Annotation>& annotations,
                                 const QString& displayTitle,
                                 const QString& listTitle) {
  if (getNDocs()) {
    getStream() << ",";
  }
  JsonLinesDocsWriter::addDocument(md5, content, metadata, annotations,
                                   displayTitle, listTitle);
}

void JsonDocsWriter::writePrefix() { getStream() << "[\n"; }

void JsonDocsWriter::writeSuffix() {
  getStream() << (getNDocs() ? "\n" : "") << "]\n";
}

LabelRecord jsonToLabelRecord(const QJsonValue& json) {
  LabelRecord record;
  auto jsonObj = json.toObject();
  record.name = jsonObj["name"].toString();
  if (jsonObj.contains("color")) {
    record.color = jsonObj["color"].toString();
  }
  if (jsonObj.contains("shortcut_key")) {
    record.shortcutKey = jsonObj["shortcut_key"].toString();
  }
  return record;
}

DatabaseCatalog::DatabaseCatalog(QObject* parent) : QObject(parent) {
  openTempDatabase(false);
}

QString getDefaultDatabasePath() {
  QSettings settings("labelbuddy", "labelbuddy");
  if (!settings.contains("last_opened_database")) {
    return QString();
  }
  auto lastOpened = settings.value("last_opened_database").toString();
  if (!QFileInfo(lastOpened).exists()) {
    return QString();
  }
  return lastOpened;
}

RemoveConnection::RemoveConnection(const QString& connectionName)
    : connectionName_{connectionName}, cancelled_{false} {}

RemoveConnection::~RemoveConnection() {
  try {
    execute();
  } catch (...) {
  }
}

void RemoveConnection::execute() const {
  if (!cancelled_ && QSqlDatabase::contains(connectionName_)) {
    QSqlDatabase::removeDatabase(connectionName_);
  }
}

void RemoveConnection::cancel() { cancelled_ = true; }

bool DatabaseCatalog::openDatabase(const QString& databasePath, bool remember) {
  QString actualDatabasePath{databasePath == QString()
                                 ? getDefaultDatabasePath()
                                 : absoluteDatabasePath(databasePath)};
  if (actualDatabasePath == QString()) {
    return false;
  }
  if (QSqlDatabase::contains(actualDatabasePath)) {
    currentDatabase_ = actualDatabasePath;
    if (remember) {
      storeDbPath(actualDatabasePath);
    }
    return true;
  }
  // Qt's "database name" the param for sqlite call
  // it is the path to db file, or "" if temporary database.
  // the argument to addDatabase is the connection name
  // we choose the db file path for non-temporary and
  // ":LABELBUDDY_TEMPORARY_DATABASE:" otherwise
  auto dbName = actualDatabasePath == tmpDbName_ ? "" : actualDatabasePath;
  bool initialized{};
  RemoveConnection removeCon(actualDatabasePath);
  {
    // scope to make sure db and queries are destroyed before attempting to
    // remove the connection if the initialization fails; see
    // https://doc.qt.io/qt-5/qsqldatabase.html#removeDatabase
    auto db = QSqlDatabase::addDatabase("QSQLITE", actualDatabasePath);
    db.setDatabaseName(dbName);
    initialized = initializeDatabase(db);
  }
  if (!initialized) {
    return false;
  }
  removeCon.cancel();
  currentDatabase_ = actualDatabasePath;
  if (remember) {
    storeDbPath(actualDatabasePath);
  }
  emit newDatabaseOpened(actualDatabasePath);
  return true;
}

bool DatabaseCatalog::isPersistentDatabase(const QString& dbPath) const {
  return !(dbPath == tmpDbName_ || dbPath == ":memory:" || dbPath == "");
}

QString
DatabaseCatalog::absoluteDatabasePath(const QString& databasePath) const {
  if (isPersistentDatabase(databasePath)) {
    return QFileInfo(databasePath).absoluteFilePath();
  }
  return databasePath;
}

bool DatabaseCatalog::storeDbPath(const QString& dbPath) const {
  if (!isPersistentDatabase(dbPath)) {
    return false;
  }
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("last_opened_database", dbPath);
  return true;
}

QString DatabaseCatalog::openTempDatabase(bool loadData) {
  openDatabase(tmpDbName_, false);
  if (loadData && (!tmpDbDataLoaded_)) {
    importLabels(":docs/demo_data/example_labels.json");
    importDocuments(":docs/demo_data/example_documents.json");
    setAppStateExtra("notebook_page", 0);
    tmpDbDataLoaded_ = true;
    temporaryDatabaseFilled(tmpDbName_);
  }
  return tmpDbName_;
}

QString DatabaseCatalog::getCurrentDatabase() const { return currentDatabase_; }

QString DatabaseCatalog::getLastOpenedDirectory() {
  QSettings settings("labelbuddy", "labelbuddy");
  if (settings.contains("last_opened_database")) {
    return parentDirectory(settings.value("last_opened_database").toString());
  }
  return QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0];
}

QVariant DatabaseCatalog::getAppStateExtra(const QString& key,
                                           const QVariant& defaultValue) const {
  QSqlQuery query(QSqlDatabase::database(currentDatabase_));
  query.prepare("select value from app_state_extra where key = :key;");
  query.bindValue(":key", key);
  query.exec();
  if (query.next()) {
    return query.value(0);
  }
  return defaultValue;
}

void DatabaseCatalog::setAppStateExtra(const QString& key,
                                       const QVariant& value) const {
  QSqlQuery query(QSqlDatabase::database(currentDatabase_));
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
acceptedAndDefaultFormats(DatabaseCatalog::Action action,
                          DatabaseCatalog::ItemKind kind) {
  switch (action) {
  case DatabaseCatalog::Action::Import:
    switch (kind) {
    case DatabaseCatalog::ItemKind::Document:
    case DatabaseCatalog::ItemKind::Label:
      return {{"txt", "json", "jsonl"}, "txt"};
    default:
      assert(false);
      return {{}, ""};
    }
  case DatabaseCatalog::Action::Export:
    switch (kind) {
    case DatabaseCatalog::ItemKind::Document:
    case DatabaseCatalog::ItemKind::Label:
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

QString DatabaseCatalog::fileExtensionErrorMessage(const QString& filePath,
                                                   Action action, ItemKind kind,
                                                   bool acceptDefault) {
  auto validAndDefault = acceptedAndDefaultFormats(action, kind);
  QFileInfo info(filePath);
  auto suffix = info.suffix();
  QString errorMsg{};
  if (validAndDefault.first.contains(suffix)) {
    return errorMsg;
  }
  QTextStream errorS(&errorMsg);
  errorS << (action == Action::Import ? "Import" : "Export") << " "
         << (kind == ItemKind::Document ? "documents" : "labels")
         << ": extension of '" << info.fileName()
         << "' not recognized.\nAccepted formats are: { "
         << validAndDefault.first.join(", ") << " }.";
  if (acceptDefault) {
    errorS << "\n\nAssuming " << validAndDefault.second << " format.";
  } else {
    errorS << "\nPlease rename the file with one of the recognized extensions.";
  }
  return errorMsg;
}

int DatabaseCatalog::insertDocRecord(const DocRecord& record,
                                     QSqlQuery& query) {
  QByteArray hash{};
  if (record.validContent) {
    query.prepare("insert into document (content, content_md5, metadata, "
                  "display_title, list_title) "
                  "values (:content, :md5, :extra, :st, :lt);");
    query.bindValue(":content", record.content);
    query.bindValue(":extra", record.metadata);
    hash = QCryptographicHash::hash(record.content.toUtf8(),
                                    QCryptographicHash::Md5);
    query.bindValue(":md5", hash);
    query.bindValue(":st", record.displayTitle != QString()
                               ? record.displayTitle
                               : QVariant());
    query.bindValue(":lt", record.listTitle != QString() ? record.listTitle
                                                         : QVariant());
    query.exec();
  } else {
    if (record.declaredMd5 == QString()) {
      return 0;
    }
    // bad chars are skipped. this is not inserted in db but used for lookup.
    hash = QByteArray::fromHex(record.declaredMd5.toUtf8());
  }
  if (record.annotations.empty()) {
    return 0;
  }
  query.prepare("select id from document where content_md5 = :md5;");
  query.bindValue(":md5", hash);
  query.exec();
  if (!query.next()) {
    return 0;
  }
  auto docId = query.value(0).toInt();
  return insertDocAnnotations(docId, record.annotations, query);
}

CharIndices DatabaseCatalog::getCharIndices(int docId, QSqlQuery& query) const {
  query.prepare("select content from document where id = :docid;");
  query.bindValue(":docid", docId);
  query.exec();
  query.next();
  return CharIndices(query.value(0).toString());
}

QMap<int, int> getUtf8ToUnicode(const CharIndices& charIndices,
                                const QList<Annotation>& annotations) {
  QList<int> byte_indices{};
  for (const auto& anno : annotations) {
    if (anno.startByte != Annotation::nullIndex) {
      byte_indices << anno.startByte;
    }
    if (anno.endByte != Annotation::nullIndex) {
      byte_indices << anno.endByte;
    }
  }
  return charIndices.utf8ToUnicode(byte_indices.cbegin(), byte_indices.cend());
}

int DatabaseCatalog::insertDocAnnotations(int docId,
                                          const QList<Annotation>& annotations,
                                          QSqlQuery& query) {
  if (annotations.isEmpty()) {
    return 0;
  }
  auto charIndices = getCharIndices(docId, query);
  auto utf8ToUnicode = getUtf8ToUnicode(charIndices, annotations);
  int nAnnotations{};
  for (const auto& annotation : annotations) {

    auto startChar = annotation.startChar;
    if (startChar == Annotation::nullIndex) {
      startChar =
          utf8ToUnicode.value(annotation.startByte, Annotation::nullIndex);
    }

    auto endChar = annotation.endChar;
    if (endChar == Annotation::nullIndex) {
      endChar = utf8ToUnicode.value(annotation.endByte, Annotation::nullIndex);
    }

    if (!(charIndices.isValidUnicodeIndex(startChar) &&
          charIndices.isValidUnicodeIndex(endChar))) {
      continue; // bad annotation
    }
    insertLabel(query, annotation.labelName);
    query.prepare("select id from label where name = :lname;");
    query.bindValue(":lname", annotation.labelName);
    query.exec();
    if (!query.next()) {
      continue; // bad label
    }
    auto labelId = query.value(0).toInt();
    query.prepare(
        "insert into annotation (doc_id, label_id, start_char, end_char, "
        "extra_data) values (:docid, :labelid, :schar, :echar, :extra);");
    query.bindValue(":docid", docId);
    query.bindValue(":labelid", labelId);
    query.bindValue(":schar", startChar);
    query.bindValue(":echar", endChar);
    query.bindValue(":extra", annotation.extraData != "" ? annotation.extraData
                                                         : QVariant());
    if (query.exec()) {
      ++nAnnotations;
    }
  }
  return nAnnotations;
}

void DatabaseCatalog::insertLabel(QSqlQuery& query, const QString& labelName,
                                  const QString& color,
                                  const QString& shortcutKey) {
  auto re = shortcutKeyPattern();
  bool validShortcut = re.match(shortcutKey).hasMatch();
  if (validShortcut) {
    query.prepare("select id from label where shortcut_key = :shortcut;");
    query.bindValue(":shortcut", shortcutKey);
    query.exec();
    if (query.next()) {
      validShortcut = false;
    }
  }
  query.prepare("insert into label (name, color, shortcut_key) values (:name, "
                ":color, :shortcut)");
  query.bindValue(":name", labelName);
  bool usedDefaultColor{};
  if (QColor::isValidColor(color)) {
    query.bindValue(":color", QColor(color).name());
  } else {
    query.bindValue(":color", suggestLabelColor(colorIndex_));
    usedDefaultColor = true;
  }
  query.bindValue(":shortcut",
                  (validShortcut ? shortcutKey : QVariant(QVariant::String)));
  if (query.exec() && usedDefaultColor) {
    ++colorIndex_;
  }
}

std::unique_ptr<DocsReader> getDocsReader(const QString& filePath) {
  std::unique_ptr<DocsReader> reader;
  auto suffix = QFileInfo(filePath).suffix();
  if (suffix == "json") {
    reader.reset(new JsonDocsReader(filePath));
  } else if (suffix == "jsonl") {
    reader.reset(new JsonLinesDocsReader(filePath));
  } else {
    reader.reset(new TxtDocsReader(filePath));
  }
  return reader;
}

ImportDocsResult DatabaseCatalog::importDocuments(const QString& filePath,
                                                  QProgressDialog* progress) {
  QSqlQuery query(QSqlDatabase::database(currentDatabase_));
  query.exec("select count(*) from document;");
  query.next();
  auto nBefore = query.value(0).toInt();
  auto reader = getDocsReader(filePath);
  if (reader->hasError()) {
    return {0, 0, reader->errorCode(), reader->errorMessage()};
  }
  if (progress != nullptr) {
    progress->setMaximum(reader->progressMax() + 1);
  }
  bool cancelled{};
  query.exec("begin transaction;");
  int nAnnotations{};
  int nDocsRead{};
  std::cout << std::endl;
  while (reader->readNext()) {
    if (progress != nullptr && progress->wasCanceled()) {
      cancelled = true;
      break;
    }
    if (reader->hasError()) {
      break;
    }
    ++nDocsRead;
    std::cout << "Read " << nDocsRead << " documents\r" << std::flush;
    nAnnotations += insertDocRecord(*(reader->getCurrentRecord()), query);
    if (progress != nullptr) {
      progress->setValue(reader->currentProgress());
    }
  }
  std::cout << std::endl;
  if (cancelled || reader->hasError()) {
    query.exec("rollback transaction");
  } else {
    query.exec("commit transaction");
  }
  query.exec("select count(*) from document;");
  query.next();
  auto nAfter = query.value(0).toInt();
  if (progress != nullptr) {
    progress->setValue(progress->maximum());
  }
  return {nAfter - nBefore, nAnnotations, reader->errorCode(),
          reader->errorMessage()};
}

ReadLabelsResult readLabels(const QString& filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return {QJsonArray{}, ErrorCode::FileSystemError, "Could not open file."};
  }
  auto suffix = QFileInfo(file).suffix();
  if (suffix == "json") {
    return readJsonLabels(file);
  }
  if (suffix == "jsonl") {
    return readJsonLinesLabels(file);
  }
  return readTxtLabels(file);
}

ReadLabelsResult readJsonLabels(QFile& file) {
  auto jsonDoc = QJsonDocument::fromJson(file.readAll());
  if (!jsonDoc.isArray()) {
    return {QJsonArray{}, ErrorCode::CriticalParsingError,
            "File does not contain a JSON array."};
  }
  return {jsonDoc.array(), ErrorCode::NoError, ""};
}

ReadLabelsResult readJsonLinesLabels(QFile& file) {
  QTextStream stream(&file);
  stream.setCodec("UTF-8");
  QJsonArray result{};
  while (!stream.atEnd()) {
    auto line = stream.readLine();
    if (line == "") {
      continue;
    }
    auto jsonDoc = QJsonDocument::fromJson(line.toUtf8());
    if (!jsonDoc.isObject()) {
      return {QJsonArray{}, ErrorCode::CriticalParsingError,
              "JSONLines error: could not parse line as a JSON object."};
    }
    result << jsonDoc.object();
  }
  return {result, ErrorCode::NoError, ""};
}

ReadLabelsResult readTxtLabels(QFile& file) {
  QJsonArray result{};
  file.setTextModeEnabled(true);
  QTextStream stream(&file);
  stream.setCodec("UTF-8");
  QJsonObject obj{};
  while (!stream.atEnd()) {
    obj["name"] = stream.readLine();
    result << obj;
  }
  return {result, ErrorCode::NoError, ""};
}

ImportLabelsResult DatabaseCatalog::importLabels(const QString& filePath) {
  QSqlQuery query(QSqlDatabase::database(currentDatabase_));
  query.exec("select count(*) from label;");
  query.next();
  auto nBefore = query.value(0).toInt();
  auto readResult = readLabels(filePath);
  if (readResult.errorCode != ErrorCode::NoError) {
    return {0, readResult.errorCode, readResult.errorMessage};
  }
  query.exec("begin transaction;");
  for (const auto& labelInfo : readResult.labels) {
    auto labelRecord = jsonToLabelRecord(labelInfo);
    insertLabel(query, labelRecord.name, labelRecord.color,
                labelRecord.shortcutKey);
  }
  query.exec("commit transaction;");
  query.exec("select count(*) from label;");
  query.next();
  auto nAfter = query.value(0).toInt();
  return {nAfter - nBefore, ErrorCode::NoError, ""};
}

std::unique_ptr<DocsWriter> getDocsWriter(const QString& filePath,
                                          bool includeText,
                                          bool includeAnnotations) {

  auto suffix = QFileInfo(filePath).suffix();
  std::unique_ptr<DocsWriter> writer{nullptr};
  if (suffix == "jsonl") {
    writer.reset(
        new JsonLinesDocsWriter(filePath, includeText, includeAnnotations));
  } else {
    writer.reset(new JsonDocsWriter(filePath, includeText, includeAnnotations));
  }
  return writer;
}

ExportDocsResult
DatabaseCatalog::exportDocuments(const QString& filePath, bool labelledDocsOnly,
                                 bool includeText, bool includeAnnotations,
                                 QProgressDialog* progress) const {
  auto writer = getDocsWriter(filePath, includeText, includeAnnotations);
  if (!writer->isOpen()) {
    return {0, 0, ErrorCode::FileSystemError, QString("Could not open file.")};
  }

  QSqlQuery query(QSqlDatabase::database(currentDatabase_));
  int totalNDocs{};
  if (labelledDocsOnly) {
    query.exec("select count(*) from labelled_document;");
    query.next();
    totalNDocs = query.value(0).toInt();
    query.exec("select id from labelled_document order by id;");
  } else {
    query.exec("select count(*) from document;");
    query.next();
    totalNDocs = query.value(0).toInt();
    query.exec("select id from document order by id;");
  }
  if (progress != nullptr) {
    progress->setMaximum(totalNDocs + 1);
  }

  int nDocs{};
  int nAnnotations{};
  writer->writePrefix();
  std::cout << std::endl;
  while (query.next()) {
    if (progress != nullptr && progress->wasCanceled()) {
      break;
    }
    ++nDocs;
    auto docId = query.value(0).toInt();
    nAnnotations += writeDoc(*writer, docId, includeText, includeAnnotations);
    if (progress != nullptr) {
      progress->setValue(nDocs);
    }
    std::cout << "Exported " << nDocs << " documents.\r" << std::flush;
  }
  std::cout << std::endl;
  writer->writeSuffix();
  if (progress != nullptr) {
    progress->setValue(progress->maximum());
  }
  return {nDocs, nAnnotations, ErrorCode::NoError, ""};
}

int DatabaseCatalog::writeDoc(DocsWriter& writer, int docId, bool includeText,
                              bool includeAnnotations) const {
  assert(includeText == writer.isIncludingText());
  assert(includeAnnotations == writer.isIncludingAnnotations());

  QSqlQuery docQuery(QSqlDatabase::database(currentDatabase_));
  docQuery.prepare("select lower(hex(content_md5)) as md5, content, "
                   "metadata, display_title, list_title "
                   "from document where id = :doc;");
  docQuery.bindValue(":doc", docId);
  docQuery.exec();
  docQuery.next();
  auto metadata =
      QJsonDocument::fromJson(docQuery.value("metadata").toByteArray())
          .object();
  auto content = docQuery.value("content").toString();
  QList<Annotation> annotations{};
  if (includeAnnotations) {
    annotations = getDocAnnotations(docId, content);
  }
  writer.addDocument(docQuery.value("md5").toString(), content, metadata,
                     annotations, docQuery.value("display_title").toString(),
                     docQuery.value("list_title").toString());
  return annotations.size();
}

QList<Annotation>
DatabaseCatalog::getDocAnnotations(int docId, const QString& content) const {
  QSqlQuery annotationsQuery(QSqlDatabase::database(currentDatabase_));
  annotationsQuery.prepare(
      "select label.name as label_name, start_char, end_char, extra_data "
      "from annotation inner join label on annotation.label_id = label.id "
      "where annotation.doc_id = :doc order by annotation.rowid;");
  annotationsQuery.bindValue(":doc", docId);
  annotationsQuery.exec();

  QList<Annotation> annotations{};
  QList<int> unicodeIndices{};
  while (annotationsQuery.next()) {
    annotations << Annotation{annotationsQuery.value("start_char").toInt(),
                              annotationsQuery.value("end_char").toInt(),
                              annotationsQuery.value("label_name").toString(),
                              annotationsQuery.value("extra_data").toString(),
                              Annotation::nullIndex,
                              Annotation::nullIndex};
    unicodeIndices << annotationsQuery.value("start_char").toInt()
                   << annotationsQuery.value("end_char").toInt();
  }
  if (annotations.size()) {
    auto unicodeToUtf8 = CharIndices(content).unicodeToUtf8(
        unicodeIndices.cbegin(), unicodeIndices.cend());
    for (auto& anno : annotations) {
      anno.startByte = unicodeToUtf8[anno.startChar];
      anno.endByte = unicodeToUtf8[anno.endChar];
    }
  }
  return annotations;
}

ExportLabelsResult
DatabaseCatalog::exportLabels(const QString& filePath) const {
  QJsonArray labels{};
  QSqlQuery query(QSqlDatabase::database(currentDatabase_));
  query.exec("select name, color, shortcut_key from sorted_label;");
  while (query.next()) {
    QJsonObject labelInfo{};
    labelInfo["name"] = query.value(0).toString();
    labelInfo["color"] = QColor(query.value(1).toString()).name();
    auto key = query.value(2);
    if (!key.isNull()) {
      labelInfo["shortcut_key"] = key.toString();
    }
    labels << labelInfo;
  }
  auto suffix = QFileInfo(filePath).suffix();
  if (suffix == "jsonl") {
    return writeLabelsToJsonLines(labels, filePath);
  }
  return writeLabelsToJson(labels, filePath);
}

ExportLabelsResult writeLabelsToJson(const QJsonArray& labels,
                                     const QString& filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    return {0, ErrorCode::FileSystemError, QString("Could not open file.")};
  }
  file.write(QJsonDocument(labels).toJson());

  return {labels.size(), ErrorCode::NoError, ""};
}

ExportLabelsResult writeLabelsToJsonLines(const QJsonArray& labels,
                                          const QString& filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    return {0, ErrorCode::FileSystemError, QString("Could not open file.")};
  }
  QTextStream stream(&file);
  stream.setCodec("UTF-8");
  for (const auto& labelInfo : labels) {
    stream << QJsonDocument(labelInfo.toObject()).toJson(QJsonDocument::Compact)
           << "\n";
  }
  return {labels.size(), ErrorCode::NoError, ""};
}

int batchImportExport(const QString& dbPath, const QList<QString>& labelsFiles,
                      const QList<QString>& docsFiles,
                      const QString& exportLabelsFile,
                      const QString& exportDocsFile, bool labelledDocsOnly,
                      bool includeText, bool includeAnnotations, bool vacuum) {
  DatabaseCatalog catalog{};
  if (!catalog.openDatabase(dbPath, false)) {
    std::cerr << "Could not open database: " << dbPath.toStdString()
              << std::endl;
    return 1;
  }
  if (vacuum) {
    catalog.vacuumDb();
    return 0;
  }
  int errors{};
  QString errorMsg{};
  for (const auto& lFile : labelsFiles) {
    errorMsg = DatabaseCatalog::fileExtensionErrorMessage(
        lFile, DatabaseCatalog::Action::Import,
        DatabaseCatalog::ItemKind::Label, false);
    if (errorMsg == QString()) {
      auto res = catalog.importLabels(lFile);
      if (res.errorCode != ErrorCode::NoError) {
        errors = 1;
      }
    } else {
      errors = 1;
      std::cerr << errorMsg.toStdString() << std::endl;
    }
  }
  for (const auto& dFile : docsFiles) {
    errorMsg = DatabaseCatalog::fileExtensionErrorMessage(
        dFile, DatabaseCatalog::Action::Import,
        DatabaseCatalog::ItemKind::Document, false);
    if (errorMsg == QString()) {
      auto res = catalog.importDocuments(dFile);
      if (res.errorCode != ErrorCode::NoError) {
        errors = 1;
      }
    } else {
      errors = 1;
      std::cerr << errorMsg.toStdString() << std::endl;
    }
  }
  if (exportLabelsFile != QString()) {
    errorMsg = DatabaseCatalog::fileExtensionErrorMessage(
        exportLabelsFile, DatabaseCatalog::Action::Export,
        DatabaseCatalog::ItemKind::Label, true);
    if (errorMsg != QString()) {
      std::cerr << errorMsg.toStdString() << std::endl;
      // still exported, so don't count it as an error
    }
    auto res = catalog.exportLabels(exportLabelsFile);
    if (res.errorCode != ErrorCode::NoError) {
      errors = 1;
    }
  }
  if (exportDocsFile != QString()) {
    errorMsg = DatabaseCatalog::fileExtensionErrorMessage(
        exportDocsFile, DatabaseCatalog::Action::Export,
        DatabaseCatalog::ItemKind::Document, true);
    if (errorMsg != QString()) {
      std::cerr << errorMsg.toStdString() << std::endl;
      // still exported, so don't count it as an error
    }
    auto res = catalog.exportDocuments(exportDocsFile, labelledDocsOnly,
                                       includeText, includeAnnotations);
    if (res.errorCode != ErrorCode::NoError) {
      errors = 1;
    }
  }
  return errors;
}

void DatabaseCatalog::vacuumDb() const {
  QSqlQuery query(QSqlDatabase::database(currentDatabase_));
  query.exec("VACUUM;");
}

bool DatabaseCatalog::initializeDatabase(QSqlDatabase& database) {
  if (!database.open()) {
    return false;
  }
  QSqlQuery query(database);
  // file not empty and is not an SQLite db
  if (!query.exec("PRAGMA schema_version")) {
    return false;
  }
  query.next();
  auto sqliteSchemaVersion = query.value(0).toInt();

  // db existed before (schema has been modified if schema version != 0)
  if (sqliteSchemaVersion != 0) {
    query.exec("PRAGMA application_id;");
    query.next();
    // not a labelbuddy db
    if (query.value(0).toInt() != sqliteApplicationId_) {
      return false;
    }
    query.exec("PRAGMA user_version;");
    query.next();
    if (query.value(0).toInt() != sqliteUserVersion_) {
      return false;
    }
    // already contains a labelbuddy db
    // check it is not readonly
    if (!query.exec(
            QString("PRAGMA application_id = %1;").arg(sqliteApplicationId_))) {
      return false;
    }
    return (query.exec("PRAGMA foreign_keys = ON;"));
  }
  if (!query.exec("PRAGMA foreign_keys = ON;")) {
    return false;
  }
  return createTables(query);
}

bool DatabaseCatalog::createTables(QSqlQuery& query) {
  query.exec("BEGIN TRANSACTION;");
  bool success{true};
  success =
      success &&
      query.exec(
          QString("PRAGMA application_id = %1;").arg(sqliteApplicationId_));
  success =
      success &&
      query.exec(QString("PRAGMA user_version = %1;").arg(sqliteUserVersion_));

  success =
      success &&
      query.exec(
          "CREATE TABLE IF NOT EXISTS document (id INTEGER PRIMARY KEY, "
          "content_md5 BLOB UNIQUE NOT NULL, "
          "content TEXT NOT NULL, metadata BLOB, "
          "list_title TEXT DEFAULT NULL, display_title TEXT DEFAULT NULL, "
          "CHECK (content != ''), CHECK (length(content_md5 = 128)));");
  // for some reason the auto index created for the primary key is not treated
  // as a covering index in 'count(*) from document where id < xxx' but this is:
  success =
      success && query.exec("CREATE INDEX IF NOT EXISTS document_id_idx ON "
                            "document(id);");

  success = success &&
            query.exec(
                "CREATE TABLE IF NOT EXISTS label(id INTEGER PRIMARY KEY, name "
                "TEXT UNIQUE NOT NULL, color TEXT NOT NULL DEFAULT '#FFA000', "
                "shortcut_key TEXT UNIQUE DEFAULT NULL, "
                "display_order INTEGER DEFAULT NULL, CHECK (name != '')); ");
  // NULLS LAST only available from sqlite 3.30
  success = success && query.exec("CREATE VIEW IF NOT EXISTS sorted_label AS "
                                  "SELECT * FROM label ORDER BY "
                                  "display_order IS NULL, display_order, id;");

  success =
      success &&
      query.exec(
          " CREATE TABLE IF NOT EXISTS annotation(doc_id NOT NULL "
          "REFERENCES document(id) ON DELETE CASCADE, label_id NOT NULL "
          "REFERENCES label(id) ON DELETE CASCADE, start_char INTEGER NOT "
          "NULL, end_char INTEGER NOT NULL, extra_data TEXT DEFAULT NULL, "
          "UNIQUE (doc_id, start_char, end_char, label_id) "
          "CHECK (0 <= start_char) "
          "CHECK (start_char < end_char)); ");
  // end_char <= length(document.content) is not checked here because it would
  // require reading the doc content into memory for every annotation insertion.
  // Instead it is checked when importing annotations, and it is the case when
  // annotations are created within labelbuddy because they come from a Qt
  // cursor position.

  success = success &&
            query.exec(" CREATE INDEX IF NOT EXISTS annotation_doc_id_idx ON "
                       "annotation(doc_id);");

  success = success &&
            query.exec("CREATE INDEX IF NOT EXISTS annotation_label_id_idx ON "
                       "annotation(label_id);");

  success =
      success &&
      query.exec(
          "CREATE TABLE IF NOT EXISTS app_state (last_visited_doc INTEGER "
          "REFERENCES document(id) ON DELETE SET NULL);");

  success =
      success &&
      query.exec(
          "INSERT INTO app_state (last_visited_doc) SELECT (null) WHERE NOT "
          "EXISTS (SELECT * from app_state); ");

  success =
      success &&
      query.exec(
          "CREATE TABLE IF NOT EXISTS app_state_extra (key TEXT UNIQUE NOT "
          "NULL, value); ");

  success = success && query.exec("CREATE TABLE IF NOT EXISTS database_info "
                                  "(database_schema_version INTEGER, "
                                  "created_by_labelbuddy_version TEXT);");

  query.prepare("INSERT INTO database_info "
                "(database_schema_version, "
                "created_by_labelbuddy_version) "
                "SELECT :uv, :lbv "
                "WHERE NOT EXISTS (SELECT * FROM database_info);");
  query.bindValue(":uv", sqliteUserVersion_);
  query.bindValue(":lbv", getVersion());
  success = success && query.exec();

  // In experiments with many documents the subqueries below seemed much
  // faster than a left join and slightly faster than using 'where not exists'.
  success =
      success &&
      query.exec(
          "CREATE VIEW IF NOT EXISTS unlabelled_document AS SELECT * FROM "
          "document WHERE id NOT IN (SELECT distinct doc_id FROM "
          "annotation); ");

  success =
      success &&
      query.exec("CREATE VIEW IF NOT EXISTS labelled_document AS SELECT * "
                 "FROM document WHERE id IN (SELECT distinct doc_id FROM "
                 "annotation); ");
  if (success) {
    query.exec("COMMIT;");
    return true;
  }
  query.exec("ROLLBACK;");
  return false;
}

} // namespace labelbuddy
