#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTextStream>

#include "test_database.h"

namespace labelbuddy {

void TestDatabase::cleanup() {
  for (const auto& connectionName : QSqlDatabase::connectionNames()) {
    QSqlDatabase::removeDatabase(connectionName);
  }
}

void TestDatabase::testAppStateExtra() {
  QTemporaryDir tmpDir{};
  DatabaseCatalog catalog{};
  auto filePath = tmpDir.filePath("db.sqlite");
  catalog.openDatabase(filePath);
  catalog.setAppStateExtra("my key", "value 1");
  auto res = catalog.getAppStateExtra("my key", "default").toString();
  QCOMPARE(res, QString("value 1"));
  res = catalog.getAppStateExtra("new key", "default").toString();
  QCOMPARE(res, QString("default"));
  catalog.setAppStateExtra("my key", "value 2");
  res = catalog.getAppStateExtra("my key", "default").toString();
  QCOMPARE(res, QString("value 2"));
}

void TestDatabase::testOpenDatabase() {
  QTemporaryDir tmpDir{};
  DatabaseCatalog catalog{};
  auto filePath = tmpDir.filePath("db.sqlite");
  catalog.openDatabase(filePath);
  auto db = QSqlDatabase::database(filePath);
  QStringList expected{"document",  "label",           "annotation",
                       "app_state", "app_state_extra", "database_info"};
  QCOMPARE(db.tables(), expected);
  QCOMPARE(catalog.getCurrentDatabase(), filePath);

  catalog.openTempDatabase();
  QCOMPARE(catalog.getLastOpenedDirectory(), tmpDir.filePath(""));
  QSqlQuery query(QSqlDatabase::database(catalog.getCurrentDatabase()));
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 4);
  query.exec("select count(*) from label where name = 'Word';");
  query.next();
  QCOMPARE(query.value(0).toInt(), 1);
  catalog.openDatabase(filePath);
  QCOMPARE(catalog.getCurrentDatabase(), filePath);
  QSqlQuery newQuery(QSqlDatabase::database(catalog.getCurrentDatabase()));
  newQuery.exec("select count(*) from label where name = 'pronoun';");
  newQuery.next();
  QCOMPARE(newQuery.value(0).toInt(), 0);
  auto filePath1 = tmpDir.filePath("db_1.sqlite");
  catalog.openDatabase(filePath1, false);
  {
    DatabaseCatalog newCatalog{};
    newCatalog.openDatabase();
    QCOMPARE(newCatalog.getCurrentDatabase(), filePath);
  }
  auto res = catalog.openDatabase(filePath1, true);
  QCOMPARE(res, true);
  {
    DatabaseCatalog newCatalog{};
    newCatalog.openDatabase();
    QCOMPARE(newCatalog.getCurrentDatabase(), filePath1);
  }
}

void TestDatabase::testOpenDatabaseErrors() {
  QTemporaryDir tmpDir{};
  DatabaseCatalog catalog{};
  auto filePath = tmpDir.filePath("db.sqlite");
  catalog.openDatabase();
  auto dbPath = catalog.getCurrentDatabase();
  QCOMPARE(QFileInfo(dbPath).fileName(),
           QString(":LABELBUDDY_TEMPORARY_DATABASE:"));
  QVERIFY(QSqlDatabase::contains(":LABELBUDDY_TEMPORARY_DATABASE:"));
  catalog.openDatabase(filePath);
  dbPath = catalog.getCurrentDatabase();
  QCOMPARE(dbPath, filePath);
  QVERIFY(QSqlDatabase::contains(filePath));

  auto badFilePath = tmpDir.filePath("bad.sql");
  {
    QFile file(badFilePath);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream ostream(&file);
    ostream << "hello\n";
  }
  bool opened = catalog.openDatabase(badFilePath);
  QVERIFY(!opened);
  QVERIFY(!QSqlDatabase::contains(badFilePath));
  dbPath = catalog.getCurrentDatabase();
  QCOMPARE(dbPath, filePath);
  badFilePath = "/tmp/some/dir/does/notexist.sql";
  opened = catalog.openDatabase(badFilePath);
  QVERIFY(!opened);
  QVERIFY(!QSqlDatabase::contains(badFilePath));
  dbPath = catalog.getCurrentDatabase();
  QCOMPARE(dbPath, filePath);

  auto badDbPath = tmpDir.filePath("other_db_type.sql");
  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", "non_labelbuddy");
    db.setDatabaseName(badDbPath);
    db.open();
    QSqlQuery query(db);
    auto executed = query.exec("create table bookmarks (page, title);");
    QVERIFY(executed);
  }
  QSqlDatabase::removeDatabase("non_labelbuddy");
  opened = catalog.openDatabase(badDbPath);
  QVERIFY(!opened);
  QVERIFY(!QSqlDatabase::contains(badDbPath));
  dbPath = catalog.getCurrentDatabase();
  QCOMPARE(dbPath, filePath);
}

void TestDatabase::testLastOpenedDatabase() {
  QTemporaryDir tmpDir{};
  auto filePath = tmpDir.filePath("database.labelbuddy");
  {
    DatabaseCatalog catalog{};
    auto opened = catalog.openDatabase(filePath);
    QVERIFY(opened);
    QCOMPARE(catalog.getCurrentDatabase(), filePath);
  }
  {
    DatabaseCatalog catalog{};
    auto opened = catalog.openDatabase();
    QVERIFY(opened);
    QCOMPARE(catalog.getCurrentDatabase(), filePath);
  }
  QFile(filePath).remove();
  {
    DatabaseCatalog catalog{};
    auto opened = catalog.openDatabase();
    QVERIFY(!opened);
    QCOMPARE(catalog.getCurrentDatabase(), ":LABELBUDDY_TEMPORARY_DATABASE:");
  }
}

void TestDatabase::testStoredDatabasePath() {
  DatabaseCatalog catalog{};
  QDir dir(".");
  auto relPath = dir.filePath("database.labelbuddy");
  auto absPath = dir.absoluteFilePath("database.labelbuddy");
  QVERIFY(relPath != absPath);
  auto opened = catalog.openDatabase(relPath);
  QVERIFY(opened);
  auto stored = QSettings("labelbuddy", "labelbuddy")
                    .value("last_opened_database")
                    .toString();
  QCOMPARE(stored, absPath);

  opened = catalog.openDatabase(":memory:");
  QVERIFY(opened);
  stored = QSettings("labelbuddy", "labelbuddy")
               .value("last_opened_database")
               .toString();
  QCOMPARE(stored, absPath);

  opened = catalog.openDatabase("");
  QVERIFY(opened);
  stored = QSettings("labelbuddy", "labelbuddy")
               .value("last_opened_database")
               .toString();
  QCOMPARE(stored, absPath);
}

void TestDatabase::testImportExportDocs_data() {
  QTest::addColumn<QString>("importFormat");
  QTest::addColumn<bool>("importWithMetadata");
  QTest::addColumn<QString>("exportFormat");
  QTest::addColumn<bool>("exportWithContent");
  QTest::addColumn<bool>("exportAnnotations");
  QTest::addColumn<bool>("exportAll");

  QList<QString> formats{"json", "jsonl"};
  QList<bool> falseTrue{false, true};
  int i{};
  QString name{};
  QTextStream ns(&name);
  for (const auto& exportFormat : formats) {
    for (auto withContent : falseTrue) {
      for (auto exportAnnotations : falseTrue) {
        for (auto exportAll : falseTrue) {
          name = QString();
          ns << "txt " << 0 << " " << exportFormat << " " << int(withContent)
             << " " << int(exportAnnotations) << " " << int(exportAll);
          QTest::newRow(name.toUtf8().data())
              << "txt" << false << exportFormat << withContent
              << exportAnnotations << exportAll;
          ++i;
          for (auto withMetadata : falseTrue) {
            for (const auto& importFormat : formats) {
              name = QString();
              ns << importFormat << " " << int(withMetadata) << " "
                 << exportFormat << " " << int(withContent) << " "
                 << int(exportAnnotations) << " " << int(exportAll);
              QTest::newRow(name.toUtf8().data())
                  << importFormat << withMetadata << exportFormat << withContent
                  << exportAnnotations << exportAll;
              ++i;
            }
          }
        }
      }
    }
  }
}

void TestDatabase::testImportExportDocs() {
  QTemporaryDir tmpDir{};
  auto docsFile = createDocumentsFile(tmpDir);

  DatabaseCatalog catalog{};
  auto filePath = tmpDir.filePath("db.sqlite");
  catalog.openDatabase(filePath);
  catalog.importDocuments(docsFile);
  catalog.importLabels(":test/data/test_labels.json");

  QSqlQuery query(QSqlDatabase::database(catalog.getCurrentDatabase()));

  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (2, 1, 3, 4);");
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char, "
             "extra_data) values (2, 2, 5, 7, 'hello extra data');");
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (3, 1, 3, 4);");

  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 6);

  auto exportFile = checkExportedDocs(catalog, tmpDir);
  checkImportBack(catalog, exportFile);
}

void TestDatabase::testImportExportLabels() {
  QTemporaryDir tmpDir{};
  DatabaseCatalog catalog{};
  auto filePath = tmpDir.filePath("db.sqlite");
  catalog.openDatabase(filePath);
  catalog.importLabels(":test/data/test_labels.json");
  QSqlQuery query(QSqlDatabase::database(catalog.getCurrentDatabase()));
  checkDbLabels(query);
  for (QString format : {"jsonl", "json"}) {
    auto labelsFilePath = tmpDir.filePath(QString("labels.%0").arg(format));
    catalog.exportLabels(labelsFilePath);
    query.exec("delete from label;");
    query.exec("select count(*) from label;");
    query.next();
    QCOMPARE(query.value(0).toInt(), 0);
    catalog.importLabels(labelsFilePath);
    checkDbLabels(query);
  }
}

void TestDatabase::testImportTxtLabels() {
  QTemporaryDir tmpDir{};
  DatabaseCatalog catalog{};
  auto filePath = tmpDir.filePath("db.sqlite");
  catalog.openDatabase(filePath);
  auto labelsFilePath = tmpDir.filePath("labels.txt");
  {
    QFile labelsFile(labelsFilePath);
    labelsFile.open(QIODevice::WriteOnly);
    labelsFile.write("First label\nSecond label");
  }
  catalog.importLabels(labelsFilePath);
  QSqlQuery query(QSqlDatabase::database(catalog.getCurrentDatabase()));
  query.exec("select name from label order by id;");
  query.next();
  QCOMPARE(query.value(0).toString(), "First label");
  query.next();
  QCOMPARE(query.value(0).toString(), "Second label");
  query.exec("select count(*) from label;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 2);
}

void TestDatabase::testImportLabelWithDuplicateShortcutKey() {
  QTemporaryDir tmpDir{};
  DatabaseCatalog catalog{};
  auto filePath = tmpDir.filePath("db.sqlite");
  catalog.openDatabase(filePath);
  auto labelsFilePath = tmpDir.filePath("labels.json");
  QFile file(labelsFilePath);
  file.open(QIODevice::WriteOnly);
  file.write(
      R"([{"name": "lab1", "shortcut_key": "a"},
 {"name": "lab2", "shortcut_key": "a"}])");
  file.close();
  catalog.importLabels(labelsFilePath);
  QSqlQuery query(QSqlDatabase::database(catalog.getCurrentDatabase()));
  query.exec("select name, shortcut_key from label order by id;");
  query.next();
  QCOMPARE(query.value(0).toString(), QString("lab1"));
  QCOMPARE(query.value(1).toString(), QString("a"));
  query.next();
  QCOMPARE(query.value(0).toString(), QString("lab2"));
  QVERIFY(query.value(1).isNull());
}

void TestDatabase::checkDbLabels(QSqlQuery& query) {
  query.exec("select name, color, shortcut_key from label;");
  query.next();
  QCOMPARE(query.value(0).toString(), QString("label: Reinício da sessão"));
  QCOMPARE(query.value(1).toString(), QString("#aec7e8"));
  QCOMPARE(query.value(2).toString(), QString("p"));
  query.next();
  QCOMPARE(query.value(0).toString(),
           QString("label: Resumption of the session"));
  QCOMPARE(query.value(1).toString(), QString("#ffbb78"));
  query.next();
  QCOMPARE(query.value(0).toString(), QString("label: Επαvάληψη της συvσδoυ"));
  QCOMPARE(query.value(1).toString(), QString("#98df8a"));
}

void TestDatabase::checkImportBack(DatabaseCatalog& catalog,
                                   const QString& exportFile) {
  QFETCH(QString, exportFormat);

  QSqlQuery query(QSqlDatabase::database(catalog.getCurrentDatabase()));
  query.exec("select count(*) from annotation;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 3);
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 6);

  catalog.importDocuments(exportFile);

  query.exec("select count(*) from annotation;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 3);
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 6);

  query.exec("delete from annotation;");

  query.exec("select count(*) from annotation;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 6);

  catalog.importDocuments(exportFile);
  QFETCH(bool, exportAnnotations);

  query.exec("select count(*) from annotation;");
  query.next();
  QCOMPARE(query.value(0).toInt(), exportAnnotations ? 3 : 0);
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 6);

  if (exportAnnotations) {
    query.exec(
        "select doc_id, label_id, start_char from annotation order by rowid;");

    query.next();
    QCOMPARE(query.value(0).toInt(), 2);
    QCOMPARE(query.value(1).toInt(), 1);
    QCOMPARE(query.value(2).toInt(), 3);

    query.next();
    QCOMPARE(query.value(0).toInt(), 2);
    QCOMPARE(query.value(1).toInt(), 2);
    QCOMPARE(query.value(2).toInt(), 5);

    query.next();
    QCOMPARE(query.value(0).toInt(), 3);
    QCOMPARE(query.value(1).toInt(), 1);
    QCOMPARE(query.value(2).toInt(), 3);
  }
  QFETCH(bool, exportWithContent);
  if (!exportWithContent) {
    return;
  }
  query.exec("delete from document;");

  query.exec("select count(*) from annotation;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);

  catalog.importDocuments(exportFile);

  query.exec("select count(*) from annotation;");
  query.next();
  QCOMPARE(query.value(0).toInt(), exportAnnotations ? 3 : 0);

  QFETCH(bool, exportAll);

  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), (exportAll ? 6 : 2));

  QTemporaryDir tmpDir{};
  if (exportAnnotations || exportAll) {
    checkExportedDocs(catalog, tmpDir);
  }
}

QString TestDatabase::checkExportedDocs(DatabaseCatalog& catalog,
                                        QTemporaryDir& tmpDir) {
  QFETCH(QString, exportFormat);
  QFETCH(bool, exportWithContent);
  QFETCH(bool, exportAnnotations);
  QFETCH(bool, exportAll);

  QFile docsFile(":test/data/test_documents.json");
  docsFile.open(QIODevice::ReadOnly);
  auto docs = QJsonDocument::fromJson(docsFile.readAll()).array();
  if (!exportAll) {
    QJsonArray annotatedDocs{};
    annotatedDocs << docs[1] << docs[2];
    docs = annotatedDocs;
  }

  auto outFile = tmpDir.filePath(QString("exported.%0").arg(exportFormat));
  catalog.exportDocuments(outFile, !exportAll, exportWithContent,
                          exportAnnotations);
  checkExportedDocsJson(outFile, docs);
  return outFile;
}

void TestDatabase::checkExportedDocsJson(const QString& filePath,
                                         const QJsonArray& docs) {
  QFETCH(bool, exportWithContent);
  QFETCH(bool, exportAnnotations);
  QFETCH(bool, importWithMetadata);
  QFETCH(QString, importFormat);
  QFETCH(QString, exportFormat);
  QFile file(filePath);
  QJsonArray outputJsonData;
  if (exportFormat == "json") {
    file.open(QIODevice::ReadOnly);
    outputJsonData = QJsonDocument::fromJson(file.readAll()).array();
  } else {
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    while (!stream.atEnd()) {
      outputJsonData
          << QJsonDocument::fromJson(stream.readLine().toUtf8()).object();
    }
  }

  int i{};
  for (const auto& doc : docs) {
    auto jsonData = doc.toObject();
    auto metadata = jsonData["metadata"].toObject();
    auto outputJson = outputJsonData[i].toObject();
    if (importFormat != "txt") {
      QCOMPARE(outputJson.value("utf8_text_md5_checksum").toString(),
               metadata.value("original_md5").toString());
    }
    if (importWithMetadata) {
      QCOMPARE(outputJson.value("metadata")
                   .toObject()
                   .value("original_md5")
                   .toString(),
               metadata.value("original_md5").toString());
      QCOMPARE(
          outputJson.value("metadata").toObject().value("title").toString(),
          metadata.value("title").toString());
    }
    if (exportWithContent) {
      for (const auto& key : {"display_title", "list_title"}) {
        if (importFormat != "txt" && jsonData.contains(key)) {
          QCOMPARE(outputJson[key].toString(), jsonData[key].toString());
        }
      }
      if (importFormat != "txt") {
        QCOMPARE(outputJson.value("text").toString(),
                 jsonData["text"].toString());
      } else {
        QCOMPARE(outputJson.value("text").toString(),
                 jsonData["text"].toString().replace("\n", "\t"));
      }
    }

    if (exportAnnotations) {
      if (metadata.value("title").toString() == "document 1") {
        auto annotation =
            outputJson.value("annotations").toArray()[0].toObject();
        QCOMPARE(annotation["start_char"].toInt(), 3);
        QCOMPARE(annotation["end_char"].toInt(), 4);
        QCOMPARE(annotation["start_byte"].toInt(), 3);
        QCOMPARE(annotation["end_byte"].toInt(), 4);
        QCOMPARE(annotation["label_name"].toString(),
                 QString("label: Reinício da sessão"));

        annotation = outputJson.value("annotations").toArray()[1].toObject();
        QCOMPARE(annotation["start_char"].toInt(), 5);
        QCOMPARE(annotation["end_char"].toInt(), 7);
        QCOMPARE(annotation["start_byte"].toInt(), 5);
        QCOMPARE(annotation["end_byte"].toInt(), 7);
        QCOMPARE(annotation["label_name"].toString(),
                 QString("label: Resumption of the session"));
        QCOMPARE(annotation["extra_data"].toString(),
                 QString("hello extra data"));
      } else if (metadata.value("title").toString() == "document 2") {
        auto annotation =
            outputJson.value("annotations").toArray()[0].toObject();
        QCOMPARE(annotation["start_char"].toInt(), 3);
        QCOMPARE(annotation["end_char"].toInt(), 4);
        QCOMPARE(annotation["start_byte"].toInt(), 3);
        QCOMPARE(annotation["end_byte"].toInt(), 4);
        QCOMPARE(annotation["label_name"].toString(),
                 QString("label: Reinício da sessão"));
      }
    } else {
      QVERIFY(!outputJson.contains("labels"));
    }
    ++i;
  }
}

QString TestDatabase::createDocumentsFile(QTemporaryDir& tmpDir) {
  QFile docsFile(":test/data/test_documents.json");
  docsFile.open(QIODevice::ReadOnly);
  auto docs = QJsonDocument::fromJson(docsFile.readAll()).array();
  QFETCH(QString, importFormat);
  auto outFile = tmpDir.filePath(QString("documents.%0").arg(importFormat));
  if (importFormat == "txt") {
    createDocumentsFileTxt(outFile, docs);
  } else {
    createDocumentsFileJson(outFile, docs);
  }
  return outFile;
}

void TestDatabase::createDocumentsFileTxt(const QString& filePath,
                                          const QJsonArray& docs) {
  QFile file(filePath);
  file.open(QIODevice::WriteOnly | QIODevice::Text);
  QTextStream out(&file);
  out.setCodec("UTF-8");
  for (const auto& doc : docs) {
    out << doc.toObject()["text"].toString().replace("\n", "\t") << "\n";
  }
}

void TestDatabase::createDocumentsFileJson(const QString& filePath,
                                           const QJsonArray& docs) {
  QFETCH(QString, importFormat);
  QFETCH(bool, importWithMetadata);
  auto jsonl{importFormat == "jsonl"};
  QFile file(filePath);
  file.open(QIODevice::WriteOnly | QIODevice::Text);
  QTextStream out(&file);
  if (!jsonl) {
    out << "[\n";
  }
  int i{};
  for (const auto& doc : docs) {
    if (i) {
      if (!jsonl) {
        out << ",";
      }
      out << "\n";
    }
    auto docObj = doc.toObject();
    QJsonObject json{};
    json.insert("text", docObj["text"].toString());
    if (importWithMetadata) {
      json.insert("metadata", docObj["metadata"].toObject());
    }
    for (const auto& key : {"display_title", "list_title"}) {
      if (docObj.contains(key)) {
        json.insert(key, docObj[key].toString());
      }
    }
    out << QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact));
    ++i;
  }
  out << (jsonl ? "\n" : "\n]\n");
}

void TestDatabase::testBatchImportExport() {
  QTemporaryDir tmpDir{};
  auto dbPath = tmpDir.filePath("db.sqlite");
  auto docsFile = tmpDir.filePath("docs.json");
  auto docsFileBadExt = tmpDir.filePath("docs.abc");
  QFile::copy(":test/data/test_documents.json", docsFile);
  QFile::copy(":test/data/test_documents.json", docsFileBadExt);
  auto labelsFile = tmpDir.filePath("labels.json");
  QFile::copy(":test/data/test_labels.json", labelsFile);
  auto labelsOut = tmpDir.filePath("exported_labels.json");
  auto docsOut = tmpDir.filePath("exported_docs.json");
  auto res = batchImportExport(
      dbPath, {labelsFile},
      {docsFile, "/some/file/does/not/exist.json", docsFileBadExt}, labelsOut,
      docsOut, false, true, true, false);
  QCOMPARE(res, 1);

  QJsonArray outputDocs;
  QFile docsOutF(docsOut);
  docsOutF.open(QIODevice::ReadOnly);
  outputDocs = QJsonDocument::fromJson(docsOutF.readAll()).array();

  QFile inputDocsF(docsFile);
  inputDocsF.open(QIODevice::ReadOnly);
  auto inputDocs = QJsonDocument::fromJson(inputDocsF.readAll()).array();

  QCOMPARE(outputDocs.size(), inputDocs.size());
  QCOMPARE(outputDocs[0].toObject()["text"], inputDocs[0].toObject()["text"]);

  res = batchImportExport(dbPath, {}, {}, "", "", false, false, false, true);
  QCOMPARE(res, 0);

  res = batchImportExport("/does/not/exist/db.sql", {}, {}, "", "", false,
                          false, false, true);
  QCOMPARE(res, 1);
}

void TestDatabase::testImportErrors_data() {
  QTest::addColumn<QString>("inputFile");
  QDir dir(":test/data/invalid_files/");
  auto inputFiles = dir.entryList({"docs_*"});
  for (const auto& fileName : inputFiles) {
    QTest::newRow(fileName.toUtf8()) << dir.filePath(fileName);
  }
}

void TestDatabase::testImportErrors() {
  QFETCH(QString, inputFile);
  DatabaseCatalog catalog{};
  QSqlQuery query(QSqlDatabase::database(catalog.getCurrentDatabase()));
  auto docRes = catalog.importDocuments(inputFile);
  QCOMPARE(static_cast<int>(docRes.errorCode),
           static_cast<int>(ErrorCode::CriticalParsingError));
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);
  auto labelRes = catalog.importLabels(inputFile);
  QCOMPARE(static_cast<int>(labelRes.errorCode),
           static_cast<int>(ErrorCode::CriticalParsingError));
  query.exec("select count(*) from label;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);
}

void TestDatabase::testBadAnnotations() {
  DatabaseCatalog catalog{};
  QDir dir(":test/data/invalid_files/");
  auto annotationsFile = dir.filePath("annotations_0.jsonl");
  catalog.importDocuments(annotationsFile);

  QSqlQuery query(QSqlDatabase::database(catalog.getCurrentDatabase()));
  query.exec("select count(*) from annotation;");
  query.next();
  // only first annotation is ok
  QCOMPARE(query.value(0).toInt(), 1);
}

} // namespace labelbuddy
