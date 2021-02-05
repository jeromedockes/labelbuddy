#include <QFile>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTextStream>
#include <QXmlStreamReader>

#include "database.h"
#include "test_database.h"

namespace labelbuddy {

void TestDatabase::test_open_database() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  auto db = QSqlDatabase::database(file_path);
  QStringList expected{"document", "label", "annotation", "app_state",
                       "app_state_extra"};
  QCOMPARE(db.tables(), expected);
  QCOMPARE(catalog.get_current_database(), file_path);
  QCOMPARE(catalog.get_current_directory(), tmp_dir.filePath(""));
}

void TestDatabase::test_import_and_export() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  catalog.import_documents(":test/data/documents.txt");
  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 4);
  catalog.import_labels(":test/data/labels.json");
  query.exec("select count(*) from label;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 3);
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (2, 1, 3, 4);");
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (2, 2, 5, 7);");
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (3, 1, 3, 4);");
  auto out_path = tmp_dir.filePath("exported.jsonl");
  catalog.export_annotations(out_path, true, true, "some user");
  QFile file(out_path);
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream input_stream(&file);
  auto as_json = input_stream.readLine().toUtf8();
  auto json_object = QJsonDocument::fromJson(as_json).object();
  QCOMPARE(json_object.value("annotation_approver").toString(),
           QString("some user"));
  QVERIFY(json_object.value("text").toString().startsWith("TWO"));
  QCOMPARE(json_object.value("labels").toArray().size(), 2);
  QCOMPARE(json_object.value("labels").toArray()[1].toArray()[2].toString(),
           QString("Label f"));

  as_json = input_stream.readLine().toUtf8();
  json_object = QJsonDocument::fromJson(as_json).object();
  QVERIFY(json_object.value("text").toString().startsWith("THREE"));
  QCOMPARE(json_object.value("labels").toArray().size(), 1);

  QVERIFY(file.atEnd());

  auto json_out_path = tmp_dir.filePath("exported.json");
  catalog.export_annotations(json_out_path, true, true, "some user");
  QFile json_file(json_out_path);
  json_file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream json_input_stream(&json_file);
  auto json_array =
      QJsonDocument::fromJson(json_input_stream.readAll().toUtf8()).array();
  QVERIFY(json_array[0].toObject().value("text").toString().startsWith("TWO"));
  QVERIFY(
      json_array[1].toObject().value("text").toString().startsWith("THREE"));
}

void TestDatabase::test_import_export_xml() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  catalog.import_documents(":test/data/documents.xml");
  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 2);
  query.exec("select content from document where id = 1;");
  query.next();
  QVERIFY(query.value(0).toString().startsWith("first xml document:"));

  auto out_path = tmp_dir.filePath("exported.jsonl");
  catalog.export_annotations(out_path, false, true, "some user");
  QFile file(out_path);
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream input_stream(&file);
  auto as_json = input_stream.readLine().toUtf8();
  auto json_object = QJsonDocument::fromJson(as_json).object();
  QCOMPARE(json_object.value("extra_data").toObject().value("doi").toString(),
           QString("4567"));

  catalog.import_labels(":test/data/labels.json");
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (1, 3, 3, 4);");
  out_path = tmp_dir.filePath("exported.xml");
  catalog.export_annotations(out_path, false, true, "some user");
  QFile xml_file(out_path);
  xml_file.open(QIODevice::ReadOnly | QIODevice::Text);
  QXmlStreamReader xml(&xml_file);
  xml.readNextStartElement();
  QCOMPARE(xml.name().toString(), QString("annotated_document_set"));
  xml.readNextStartElement();
  QCOMPARE(xml.name().toString(), QString("annotated_document"));
  QCOMPARE(xml.attributes().value("doi").toString(), QString("4567"));
  xml.readNextStartElement();
  QCOMPARE(xml.name().toString(), QString("document_md5_checksum"));
  QCOMPARE(xml.readElementText(), QString("7e47eff7ba01ed7031bc6a162440cdba"));
  xml.readNextStartElement();
  QCOMPARE(xml.name().toString(), QString("document"));
  QVERIFY(xml.readElementText().startsWith("first xml document:\n\n  f"));
  xml.readNextStartElement();
  QCOMPARE(xml.name().toString(), QString("annotation_approver"));
  QCOMPARE(xml.readElementText(), QString("some user"));
  xml.readNextStartElement();
  QCOMPARE(xml.name().toString(), QString("annotation_set"));
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
  QCOMPARE(xml.readElementText(), QString("label a"));
}

void TestDatabase::test_import_json() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  catalog.import_documents(":test/data/documents.json");
  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 2);
  query.exec("select content from document where id = 1;");
  query.next();
  QVERIFY(query.value(0).toString().startsWith("ONE JSON"));
}

void TestDatabase::test_import_json_lines() {
  QTemporaryDir tmp_dir{};
  DatabaseCatalog catalog{};
  auto file_path = tmp_dir.filePath("db.sqlite");
  catalog.open_database(file_path);
  catalog.import_documents(":test/data/documents.jsonl");
  QSqlQuery query(QSqlDatabase::database(catalog.get_current_database()));
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 3);
  query.exec("select content from document where id = 3;");
  query.next();
  QVERIFY(query.value(0).toString().startsWith("THREE JSONL"));
}

} // namespace labelbuddy
