#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTextStream>
#include <QXmlStreamReader>

#include "csv.h"
#include "test_database.h"

namespace labelbuddy {

void TestDatabase::cleanup() {
  for (const auto& connection_name : QSqlDatabase::connectionNames()) {
    QSqlDatabase::removeDatabase(connection_name);
  }
}

void TestDatabase::test_app_state_extra() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  catalog.set_app_state_extra("my key", "value 1");
  auto res = catalog.get_app_state_extra("my key", "default").toString();
  QCOMPARE(res, QString("value 1"));
  res = catalog.get_app_state_extra("new key", "default").toString();
  QCOMPARE(res, QString("default"));
  catalog.set_app_state_extra("my key", "value 2");
  res = catalog.get_app_state_extra("my key", "default").toString();
  QCOMPARE(res, QString("value 2"));
}

void TestDatabase::test_open_database() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  auto db = QSqlDatabase::database(file_path);
  QStringList expected{"document",  "label",           "annotation",
                       "app_state", "app_state_extra", "database_info"};
  QCOMPARE(db.tables(), expected);
  QCOMPARE(catalog.get_current_database(), file_path);

  catalog.open_temp_database();
  QCOMPARE(catalog.get_last_opened_directory(), tmp_dir.filePath(""));
  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 4);
  query.exec("select count(*) from label where name = 'Word';");
  query.next();
  QCOMPARE(query.value(0).toInt(), 1);
  catalog.open_database(file_path);
  QCOMPARE(catalog.get_current_database(), file_path);
  QSqlQuery new_query(QSqlDatabase::database(catalog.get_current_database()));
  new_query.exec("select count(*) from label where name = 'pronoun';");
  new_query.next();
  QCOMPARE(new_query.value(0).toInt(), 0);
  auto file_path_1 = tmp_dir.filePath("db_1.sqlite");
  catalog.open_database(file_path_1, false);
  {
    DatabaseCatalog new_catalog{};
    new_catalog.open_database();
    QCOMPARE(new_catalog.get_current_database(), file_path);
  }
  auto res = catalog.open_database(file_path_1, true);
  QCOMPARE(res, true);
  {
    DatabaseCatalog new_catalog{};
    new_catalog.open_database();
    QCOMPARE(new_catalog.get_current_database(), file_path_1);
  }
}

void TestDatabase::test_open_database_errors() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database();
  auto db_path = catalog.get_current_database();
  QCOMPARE(QFileInfo(db_path).fileName(),
           QString(":LABELBUDDY_TEMPORARY_DATABASE:"));
  QVERIFY(QSqlDatabase::contains(":LABELBUDDY_TEMPORARY_DATABASE:"));
  catalog.open_database(file_path);
  db_path = catalog.get_current_database();
  QCOMPARE(db_path, file_path);
  QVERIFY(QSqlDatabase::contains(file_path));

  auto bad_file_path = tmp_dir.filePath("bad.sql");
  {
    QFile file(bad_file_path);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream ostream(&file);
    ostream << "hello\n";
  }
  bool opened = catalog.open_database(bad_file_path);
  QVERIFY(!opened);
  QVERIFY(!QSqlDatabase::contains(bad_file_path));
  db_path = catalog.get_current_database();
  QCOMPARE(db_path, file_path);
  bad_file_path = "/tmp/some/dir/does/notexist.sql";
  opened = catalog.open_database(bad_file_path);
  QVERIFY(!opened);
  QVERIFY(!QSqlDatabase::contains(bad_file_path));
  db_path = catalog.get_current_database();
  QCOMPARE(db_path, file_path);

  auto bad_db_path = tmp_dir.filePath("other_db_type.sql");
  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", "non_labelbuddy");
    db.setDatabaseName(bad_db_path);
    db.open();
    QSqlQuery query(db);
    auto executed = query.exec("create table bookmarks (page, title);");
    QVERIFY(executed);
  }
  QSqlDatabase::removeDatabase("non_labelbuddy");
  opened = catalog.open_database(bad_db_path);
  QVERIFY(!opened);
  QVERIFY(!QSqlDatabase::contains(bad_db_path));
  db_path = catalog.get_current_database();
  QCOMPARE(db_path, file_path);
}

void TestDatabase::test_last_opened_database() {
  QTemporaryDir tmp_dir{};
  auto file_path = tmp_dir.filePath("database.labelbuddy");
  {
    DatabaseCatalog catalog{};
    auto opened = catalog.open_database(file_path);
    QVERIFY(opened);
    QCOMPARE(catalog.get_current_database(), file_path);
  }
  {
    DatabaseCatalog catalog{};
    auto opened = catalog.open_database();
    QVERIFY(opened);
    QCOMPARE(catalog.get_current_database(), file_path);
  }
  QFile(file_path).remove();
  {
    DatabaseCatalog catalog{};
    auto opened = catalog.open_database();
    QVERIFY(!opened);
    QCOMPARE(catalog.get_current_database(), ":LABELBUDDY_TEMPORARY_DATABASE:");
  }
}

void TestDatabase::test_stored_database_path() {
  DatabaseCatalog catalog{};
  QDir dir(".");
  auto rel_path = dir.filePath("database.labelbuddy");
  auto abs_path = dir.absoluteFilePath("database.labelbuddy");
  QVERIFY(rel_path != abs_path);
  auto opened = catalog.open_database(rel_path);
  QVERIFY(opened);
  auto stored = QSettings("labelbuddy", "labelbuddy")
                    .value("last_opened_database")
                    .toString();
  QCOMPARE(stored, abs_path);

  opened = catalog.open_database(":memory:");
  QVERIFY(opened);
  stored = QSettings("labelbuddy", "labelbuddy")
               .value("last_opened_database")
               .toString();
  QCOMPARE(stored, abs_path);

  opened = catalog.open_database("");
  QVERIFY(opened);
  stored = QSettings("labelbuddy", "labelbuddy")
               .value("last_opened_database")
               .toString();
  QCOMPARE(stored, abs_path);
}

void TestDatabase::test_import_export_docs_data() {
  QTest::addColumn<QString>("import_format");
  QTest::addColumn<bool>("import_with_meta");
  QTest::addColumn<QString>("export_format");
  QTest::addColumn<bool>("export_with_content");
  QTest::addColumn<bool>("export_annotations");
  QTest::addColumn<bool>("export_all");

  QList<QString> formats{"json", "jsonl", "xml", "csv"};
  QList<bool> false_true{false, true};
  int i{};
  QString name{};
  QTextStream ns(&name);
  for (const auto& export_format : formats) {
    for (auto with_content : false_true) {
      for (auto export_annotations : false_true) {
        for (auto export_all : false_true) {
          name = QString();
          ns << "txt " << 0 << " " << export_format << " " << int(with_content)
             << " " << int(export_annotations) << " " << int(export_all);
          QTest::newRow(name.toUtf8().data())
              << "txt" << false << export_format << with_content
              << export_annotations << export_all;
          ++i;
          for (auto with_meta : false_true) {
            for (const auto& import_format : formats) {
              name = QString();
              ns << import_format << " " << int(with_meta) << " "
                 << export_format << " " << int(with_content) << " "
                 << int(export_annotations) << " " << int(export_all);
              QTest::newRow(name.toUtf8().data())
                  << import_format << with_meta << export_format << with_content
                  << export_annotations << export_all;
              ++i;
            }
          }
        }
      }
    }
  }
}

void TestDatabase::test_import_export_docs() {
  QTemporaryDir tmp_dir{};
  auto docs_file = create_documents_file(tmp_dir);

  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  catalog.import_documents(docs_file);
  catalog.import_labels(":test/data/test_labels.json");

  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));

  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (2, 1, 3, 4);");
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char, "
             "extra_data) values (2, 2, 5, 7, 'hello extra data');");
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (3, 1, 3, 4);");

  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 6);

  auto export_file = check_exported_docs(catalog, tmp_dir);
  check_import_back(catalog, export_file);
}

void TestDatabase::test_import_export_labels() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  catalog.import_labels(":test/data/test_labels.json");
  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  check_db_labels(query);
  for (QString format : {"xml", "jsonl", "json", "csv"}) {
    auto labels_file_path = tmp_dir.filePath(QString("labels.%0").arg(format));
    catalog.export_labels(labels_file_path);
    query.exec("delete from label;");
    query.exec("select count(*) from label;");
    query.next();
    QCOMPARE(query.value(0).toInt(), 0);
    catalog.import_labels(labels_file_path);
    check_db_labels(query);
    if (format == "csv") {
      QFile file(labels_file_path);
      file.open(QIODevice::ReadOnly);
      QTextStream stream(&file);
      CsvMapReader csv(&stream);
      QList<QString> expected{"ignore_this_column", "text", "color",
                              "shortcut_key"};
      QCOMPARE(csv.header(), expected);
    }
  }
}

void TestDatabase::test_import_txt_labels() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  auto labels_file_path = tmp_dir.filePath("labels.txt");
  {
    QFile labels_file(labels_file_path);
    labels_file.open(QIODevice::WriteOnly);
    labels_file.write("First label\nSecond label");
  }
  catalog.import_labels(labels_file_path);
  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  query.exec("select name from label order by id;");
  query.next();
  QCOMPARE(query.value(0).toString(), "First label");
  query.next();
  QCOMPARE(query.value(0).toString(), "Second label");
  query.exec("select count(*) from label;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 2);
}

void TestDatabase::test_import_label_with_duplicate_shortcut_key() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  auto labels_file_path = tmp_dir.filePath("labels.json");
  QFile file(labels_file_path);
  file.open(QIODevice::WriteOnly);
  file.write(
      R"([{"text": "lab1", "shortcut_key": "a"},
 {"text": "lab2", "shortcut_key": "a"}])");
  file.close();
  catalog.import_labels(labels_file_path);
  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  query.exec("select name, shortcut_key from label order by id;");
  query.next();
  QCOMPARE(query.value(0).toString(), QString("lab1"));
  QCOMPARE(query.value(1).toString(), QString("a"));
  query.next();
  QCOMPARE(query.value(0).toString(), QString("lab2"));
  QVERIFY(query.value(1).isNull());
}

void TestDatabase::check_db_labels(QSqlQuery& query) {
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

void TestDatabase::check_import_back(DatabaseCatalog& catalog,
                                     const QString& export_file) {
  QFETCH(QString, export_format);

  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  query.exec("select count(*) from annotation;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 3);
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 6);

  catalog.import_documents(export_file);

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

  catalog.import_documents(export_file);
  QFETCH(bool, export_annotations);

  query.exec("select count(*) from annotation;");
  query.next();
  QCOMPARE(query.value(0).toInt(), export_annotations ? 3 : 0);
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 6);

  if (export_annotations) {
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
  QFETCH(bool, export_with_content);
  if (!export_with_content) {
    return;
  }
  query.exec("delete from document;");

  query.exec("select count(*) from annotation;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);

  catalog.import_documents(export_file);

  query.exec("select count(*) from annotation;");
  query.next();
  QCOMPARE(query.value(0).toInt(), export_annotations ? 3 : 0);

  QFETCH(bool, export_all);

  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), (export_all ? 6 : 2));

  QTemporaryDir tmp_dir{};
  if (export_annotations || export_all) {
    check_exported_docs(catalog, tmp_dir);
  }
}

QString TestDatabase::check_exported_docs(DatabaseCatalog& catalog,
                                          QTemporaryDir& tmp_dir) {
  QFETCH(QString, export_format);
  QFETCH(bool, export_with_content);
  QFETCH(bool, export_annotations);
  QFETCH(bool, export_all);

  QFile docs_file(":test/data/test_documents.json");
  docs_file.open(QIODevice::ReadOnly);
  auto docs = QJsonDocument::fromJson(docs_file.readAll()).array();
  if (!export_all) {
    QJsonArray annotated_docs{};
    annotated_docs << docs[1] << docs[2];
    docs = annotated_docs;
  }

  auto out_file = tmp_dir.filePath(QString("exported.%0").arg(export_format));
  catalog.export_documents(out_file, !export_all, export_with_content,
                           export_annotations, "some_user");
  if (export_format == "xml") {
    check_exported_docs_xml(out_file, docs);
  } else if (export_format == "csv") {
    check_exported_docs_csv(out_file, docs);
  } else {
    check_exported_docs_json(out_file, docs);
  }
  return out_file;
}

void TestDatabase::check_exported_docs_xml(const QString& file_path,
                                           const QJsonArray& docs) {
  QFETCH(bool, export_with_content);
  QFETCH(bool, export_annotations);
  QFETCH(bool, import_with_meta);
  QFETCH(QString, import_format);
  QFile xml_file(file_path);
  xml_file.open(QIODevice::ReadOnly | QIODevice::Text);
  QXmlStreamReader xml(&xml_file);

  xml.readNextStartElement();
  QCOMPARE(xml.name().toString(), QString("document_set"));

  for (const auto& doc : docs) {
    auto json_data = doc.toObject();
    auto meta = json_data["meta"].toObject();
    while (!xml.readNextStartElement() && !xml.atEnd()) {
    }
    QCOMPARE(xml.name().toString(), QString("document"));

    xml.readNextStartElement();
    QCOMPARE(xml.name().toString(), QString("utf8_text_md5_checksum"));
    auto output_md5 = xml.readElementText();
    if (import_format != "txt") {
      QCOMPARE(output_md5, meta.value("original_md5").toString());
    }

    xml.readNextStartElement();
    QCOMPARE(xml.name().toString(), QString("meta"));
    if (import_with_meta) {
      QCOMPARE(xml.attributes().value("original_md5").toString(),
               meta.value("original_md5").toString());
      QCOMPARE(xml.attributes().value("title").toString(),
               meta.value("title").toString());
      QCOMPARE(xml.readElementText(), QString());
    }
    xml.readElementText();
    if (import_format != "txt" && json_data.contains("id")) {
      xml.readNextStartElement();
      QCOMPARE(xml.name().toString(), QString("id"));
      QCOMPARE(xml.readElementText(), json_data["id"].toString());
    }

    xml.readNextStartElement();
    QCOMPARE(xml.name().toString(), QString("annotation_approver"));
    QCOMPARE(xml.readElementText(), QString("some_user"));

    if (export_with_content) {
      for (const auto& key : {"short_title", "long_title"}) {
        if (import_format != "txt" && json_data.contains(key)) {
          xml.readNextStartElement();
          QCOMPARE(xml.name().toString(), key);
          QCOMPARE(xml.readElementText(), json_data[key].toString());
        }
      }
      xml.readNextStartElement();
      QCOMPARE(xml.name().toString(), QString("text"));
      auto text = xml.readElementText();
      if (import_format != "txt") {
        QCOMPARE(text, json_data["text"].toString());
      } else {
        QCOMPARE(text, json_data["text"].toString().replace("\n", "\t"));
      }
    }
    if (export_annotations) {
      xml.readNextStartElement();
      QCOMPARE(xml.name().toString(), QString("labels"));
      if (meta.value("title").toString() == "document 1") {
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("annotation"));
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("start_char"));
        QCOMPARE(xml.readElementText().toInt(), 3);
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("end_char"));
        QCOMPARE(xml.readElementText().toInt(), 4);
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("label"));
        QCOMPARE(xml.readElementText(), QString("label: Reinício da sessão"));

        xml.readNextStartElement();
        while (!xml.readNextStartElement() && !xml.atEnd()) {
        }
        QCOMPARE(xml.name().toString(), QString("annotation"));
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("start_char"));
        QCOMPARE(xml.readElementText().toInt(), 5);
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("end_char"));
        QCOMPARE(xml.readElementText().toInt(), 7);
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("label"));
        QCOMPARE(xml.readElementText(),
                 QString("label: Resumption of the session"));
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("extra_data"));
        QCOMPARE(xml.readElementText(), QString("hello extra data"));
      } else if (meta.value("title").toString() == "document 2") {
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("annotation"));
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("start_char"));
        QCOMPARE(xml.readElementText().toInt(), 3);
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("end_char"));
        QCOMPARE(xml.readElementText().toInt(), 4);
        xml.readNextStartElement();
        QCOMPARE(xml.name().toString(), QString("label"));
        QCOMPARE(xml.readElementText(), QString("label: Reinício da sessão"));
      }
    }
  }
}

void TestDatabase::check_exported_docs_json(const QString& file_path,
                                            const QJsonArray& docs) {
  QFETCH(bool, export_with_content);
  QFETCH(bool, export_annotations);
  QFETCH(bool, import_with_meta);
  QFETCH(QString, import_format);
  QFETCH(QString, export_format);
  QFile file(file_path);
  QJsonArray output_json_data;
  if (export_format == "json") {
    file.open(QIODevice::ReadOnly);
    output_json_data = QJsonDocument::fromJson(file.readAll()).array();
  } else {
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    while (!stream.atEnd()) {
      output_json_data
          << QJsonDocument::fromJson(stream.readLine().toUtf8()).object();
    }
  }

  int i{};
  for (const auto& doc : docs) {
    auto json_data = doc.toObject();
    auto meta = json_data["meta"].toObject();
    auto output_json = output_json_data[i].toObject();
    if (import_format != "txt") {
      QCOMPARE(output_json.value("utf8_text_md5_checksum").toString(),
               meta.value("original_md5").toString());
    }
    if (import_with_meta) {
      QCOMPARE(
          output_json.value("meta").toObject().value("original_md5").toString(),
          meta.value("original_md5").toString());
      QCOMPARE(output_json.value("meta").toObject().value("title").toString(),
               meta.value("title").toString());
    }
    if (import_format != "txt" && json_data.contains("id")) {
      QCOMPARE(output_json.value("id").toString(), json_data["id"].toString());
    }
    if (export_with_content) {
      for (const auto& key : {"short_title", "long_title"}) {
        if (import_format != "txt" && json_data.contains(key)) {
          QCOMPARE(output_json[key].toString(), json_data[key].toString());
        }
      }
      if (import_format != "txt") {
        QCOMPARE(output_json.value("text").toString(),
                 json_data["text"].toString());
      } else {
        QCOMPARE(output_json.value("text").toString(),
                 json_data["text"].toString().replace("\n", "\t"));
      }
    }
    QCOMPARE(output_json.value("annotation_approver").toString(),
             QString("some_user"));

    if (export_annotations) {
      if (meta.value("title").toString() == "document 1") {
        auto annotation = output_json.value("labels").toArray()[0].toArray();
        QCOMPARE(annotation[0].toInt(), 3);
        QCOMPARE(annotation[1].toInt(), 4);
        QCOMPARE(annotation[2].toString(),
                 QString("label: Reinício da sessão"));

        annotation = output_json.value("labels").toArray()[1].toArray();
        QCOMPARE(annotation[0].toInt(), 5);
        QCOMPARE(annotation[1].toInt(), 7);
        QCOMPARE(annotation[2].toString(),
                 QString("label: Resumption of the session"));
        QCOMPARE(annotation[3].toString(), QString("hello extra data"));
      } else if (meta.value("title").toString() == "document 2") {
        auto annotation = output_json.value("labels").toArray()[0].toArray();
        QCOMPARE(annotation[0].toInt(), 3);
        QCOMPARE(annotation[1].toInt(), 4);
        QCOMPARE(annotation[2].toString(),
                 QString("label: Reinício da sessão"));
      }
    } else {
      QVERIFY(!output_json.contains("labels"));
    }
    ++i;
  }
}

void TestDatabase::check_exported_docs_csv(const QString& file_path,
                                           const QJsonArray& docs) {
  QFETCH(bool, export_with_content);
  QFETCH(bool, export_annotations);
  QFETCH(bool, import_with_meta);
  QFETCH(QString, import_format);
  QFETCH(QString, export_format);
  QFile file(file_path);
  file.open(QIODevice::ReadOnly);
  QTextStream stream(&file);
  CsvMapReader reader(&stream);
  int i{};
  for (const auto& doc : docs) {
    auto json_data = doc.toObject();
    auto meta = json_data["meta"].toObject();
    auto output_doc = reader.read_record();
    if (import_format != "txt") {
      QCOMPARE(output_doc.value("utf8_text_md5_checksum"),
               meta.value("original_md5").toString());
    }
    if (import_with_meta) {
      auto output_meta =
          QJsonDocument::fromJson(output_doc["meta"].toUtf8()).object();
      QCOMPARE(output_meta.value("original_md5").toString(),
               meta.value("original_md5").toString());
      QCOMPARE(output_meta.value("title").toString(),
               meta.value("title").toString());
    }
    if (import_format != "txt" && json_data.contains("id")) {
      QCOMPARE(output_doc["id"], json_data["id"].toString());
    }

    if (export_with_content) {
      for (const auto& key : {"short_title", "long_title"}) {
        if (import_format != "txt" && json_data.contains(key)) {
          QCOMPARE(output_doc[key], json_data[key].toString());
        }
      }
      if (import_format != "txt") {
        QCOMPARE(output_doc.value("text"), json_data["text"].toString());
      } else {
        QCOMPARE(output_doc.value("text"),
                 json_data["text"].toString().replace("\n", "\t"));
      }
    }
    QCOMPARE(output_doc.value("annotation_approver"), QString("some_user"));

    if (export_annotations) {
      if (meta.value("title").toString() == "document 1") {
        QCOMPARE(output_doc["start_char"].toInt(), 3);
        QCOMPARE(output_doc["end_char"].toInt(), 4);
        QCOMPARE(output_doc["label"], QString("label: Reinício da sessão"));
        output_doc = reader.read_record();
        QCOMPARE(output_doc["start_char"].toInt(), 5);
        QCOMPARE(output_doc["end_char"].toInt(), 7);
        QCOMPARE(output_doc["label"],
                 QString("label: Resumption of the session"));
        QCOMPARE(output_doc["extra_data"], QString("hello extra data"));
      } else if (meta.value("title").toString() == "document 2") {
        QCOMPARE(output_doc["start_char"].toInt(), 3);
        QCOMPARE(output_doc["end_char"].toInt(), 4);
        QCOMPARE(output_doc["label"], QString("label: Reinício da sessão"));
      }
    } else {
      QVERIFY(!output_doc.contains("start_char"));
    }
    ++i;
  }
}

QString TestDatabase::create_documents_file(QTemporaryDir& tmp_dir) {
  QFile docs_file(":test/data/test_documents.json");
  docs_file.open(QIODevice::ReadOnly);
  auto docs = QJsonDocument::fromJson(docs_file.readAll()).array();
  QFETCH(QString, import_format);
  auto out_file = tmp_dir.filePath(QString("documents.%0").arg(import_format));
  if (import_format == "txt") {
    create_documents_file_txt(out_file, docs);
  } else if (import_format == "xml") {
    create_documents_file_xml(out_file, docs);
  } else if (import_format == "csv") {
    create_documents_file_csv(out_file, docs);
  } else {
    create_documents_file_json(out_file, docs);
  }
  return out_file;
}

void TestDatabase::create_documents_file_txt(const QString& file_path,
                                             const QJsonArray& docs) {
  QFile file(file_path);
  file.open(QIODevice::WriteOnly | QIODevice::Text);
  QTextStream out(&file);
  out.setCodec("UTF-8");
  for (const auto& doc : docs) {
    out << doc.toObject()["text"].toString().replace("\n", "\t") << "\n";
  }
}

void TestDatabase::create_documents_file_csv(const QString& file_path,
                                             const QJsonArray& docs) {
  QFETCH(bool, import_with_meta);
  QFile file(file_path);
  file.open(QIODevice::WriteOnly);
  QTextStream stream(&file);
  CsvWriter writer(&stream);
  QList<QString> row{};
  if (import_with_meta) {
    row << "meta";
  }
  row << "id"
      << "short_title"
      << "long_title"
      << "text";
  writer.write_record(row.constBegin(), row.constEnd());
  for (const auto& doc : docs) {
    auto doc_obj = doc.toObject();
    row.clear();
    if (import_with_meta) {
      row << QJsonDocument(doc_obj["meta"].toObject()).toJson();
    }
    row << doc_obj["id"].toString() << doc_obj["short_title"].toString()
        << doc_obj["long_title"].toString() << doc_obj["text"].toString();
    writer.write_record(row.constBegin(), row.constEnd());
  }
}

void TestDatabase::create_documents_file_xml(const QString& file_path,
                                             const QJsonArray& docs) {
  QFETCH(bool, import_with_meta);
  QFile file(file_path);
  file.open(QIODevice::WriteOnly | QIODevice::Text);
  QXmlStreamWriter xml(&file);
  xml.setAutoFormatting(true);
  xml.writeStartDocument();
  xml.writeStartElement("document_set");

  for (const auto& doc : docs) {
    auto doc_obj = doc.toObject();
    xml.writeStartElement("document");
    if (import_with_meta) {
      xml.writeStartElement("meta");
      auto metadata = doc_obj["meta"].toObject();
      for (auto key_val = metadata.constBegin(); key_val != metadata.constEnd();
           ++key_val) {
        xml.writeAttribute(key_val.key(),
                           key_val.value().toVariant().toString());
      }
      xml.writeEndElement();
    }
    for (const auto& key : {"id", "short_title", "long_title"}) {
      if (doc_obj.contains(key)) {
        xml.writeTextElement(key, doc_obj[key].toString());
      }
    }
    xml.writeTextElement("text", doc_obj["text"].toString());
    xml.writeEndElement();
  }

  xml.writeEndElement();
  xml.writeEndDocument();
}

void TestDatabase::create_documents_file_json(const QString& file_path,
                                              const QJsonArray& docs) {
  QFETCH(QString, import_format);
  QFETCH(bool, import_with_meta);
  auto jsonl{import_format == "jsonl"};
  QFile file(file_path);
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
    auto doc_obj = doc.toObject();
    QJsonObject json{};
    json.insert("text", doc_obj["text"].toString());
    if (import_with_meta) {
      json.insert("meta", doc_obj["meta"].toObject());
    }
    for (const auto& key : {"id", "short_title", "long_title"}) {
      if (doc_obj.contains(key)) {
        json.insert(key, doc_obj[key].toString());
      }
    }
    out << QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact));
    ++i;
  }
  out << (jsonl ? "\n" : "\n]\n");
}

void TestDatabase::test_batch_import_export() {
  QTemporaryDir tmp_dir{};
  auto db_path = tmp_dir.filePath("db.sqlite");
  auto docs_file = tmp_dir.filePath("docs.json");
  auto docs_file_bad_ext = tmp_dir.filePath("docs.abc");
  QFile::copy(":test/data/test_documents.json", docs_file);
  QFile::copy(":test/data/test_documents.json", docs_file_bad_ext);
  auto labels_file = tmp_dir.filePath("labels.json");
  QFile::copy(":test/data/test_labels.json", labels_file);
  auto labels_out = tmp_dir.filePath("exported_labels.json");
  auto docs_out = tmp_dir.filePath("exported_docs.json");
  auto res = batch_import_export(
      db_path, {labels_file},
      {docs_file, "/some/file/does/not/exist.xml", docs_file_bad_ext},
      labels_out, docs_out, false, true, true, "someone", false);
  QCOMPARE(res, 1);

  QJsonArray output_docs;
  QFile docs_out_f(docs_out);
  docs_out_f.open(QIODevice::ReadOnly);
  output_docs = QJsonDocument::fromJson(docs_out_f.readAll()).array();

  QFile input_docs_f(docs_file);
  input_docs_f.open(QIODevice::ReadOnly);
  auto input_docs = QJsonDocument::fromJson(input_docs_f.readAll()).array();

  QCOMPARE(output_docs.size(), input_docs.size());
  QCOMPARE(output_docs[0].toObject()["text"], input_docs[0].toObject()["text"]);

  res = batch_import_export(db_path, {}, {}, "", "", false, false, false, "",
                            true);
  QCOMPARE(res, 0);

  res = batch_import_export("/does/not/exist/db.sql", {}, {}, "", "", false,
                            false, false, "", true);
  QCOMPARE(res, 1);
}

void TestDatabase::test_import_errors_data() {
  QTest::addColumn<QString>("input_file");
  QDir dir(":test/data/invalid_files/");
  auto input_files = dir.entryList({"*"});
  for (const auto& file_name : input_files) {
    QTest::newRow(file_name.toUtf8()) << dir.filePath(file_name);
  }
}

void TestDatabase::test_import_errors() {
  QFETCH(QString, input_file);
  DatabaseCatalog catalog{};
  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  auto doc_res = catalog.import_documents(input_file);
  QCOMPARE(static_cast<int>(doc_res.error_code),
           static_cast<int>(ErrorCode::CriticalParsingError));
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);
  auto label_res = catalog.import_labels(input_file);
  QCOMPARE(static_cast<int>(label_res.error_code),
           static_cast<int>(ErrorCode::CriticalParsingError));
  query.exec("select count(*) from label;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);
}

} // namespace labelbuddy
