#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTextStream>
#include <QXmlStreamReader>

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

void TestDatabase::test_import_export_docs_data() {
  QTest::addColumn<QString>("import_format");
  QTest::addColumn<bool>("as_json_objects");
  QTest::addColumn<bool>("import_with_meta");
  QTest::addColumn<QString>("export_format");
  QTest::addColumn<bool>("export_with_content");
  QTest::addColumn<bool>("export_all");

  QList<QString> json_import_formats{"json", "jsonl"};
  QList<QString> export_formats{"json", "jsonl", "xml"};
  QList<bool> false_true{false, true};
  int i{};
  for (auto export_format : export_formats) {
    for (auto with_content : false_true) {
      for (auto export_all : false_true) {
        QTest::newRow(QString("row %0").arg(i).toUtf8().data())
            << "txt" << false << false << export_format << with_content
            << export_all;
        ++i;
        for (auto with_meta : false_true) {
          QTest::newRow(QString("row %0").arg(i).toUtf8().data())
              << "xml" << false << with_meta << export_format << with_content
              << export_all;
          ++i;
          for (auto import_format : json_import_formats) {
            for (auto as_objects : false_true) {
              QTest::newRow(QString("row %0").arg(i).toUtf8().data())
                  << import_format << as_objects << with_meta << export_format
                  << with_content << export_all;
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
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (2, 2, 5, 7);");
  query.exec("insert into annotation (doc_id, label_id, start_char, end_char) "
             "values (3, 1, 3, 4);");

  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 6);

  check_exported_docs(catalog, tmp_dir);
}

void TestDatabase::check_exported_docs(DatabaseCatalog& catalog,
                                       QTemporaryDir& tmp_dir) {
  QFETCH(QString, export_format);
  QFETCH(bool, export_with_content);
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
  catalog.export_annotations(out_file, !export_all, export_with_content,
                             "some_user");
  if (export_format == "xml") {
    check_exported_docs_xml(out_file, docs);
  } else {
    check_exported_docs_json(out_file, docs);
  }
}

void TestDatabase::check_exported_docs_xml(const QString& file_path,
                                           const QJsonArray& docs) {
  QFETCH(bool, export_with_content);
  QFETCH(bool, import_with_meta);
  QFETCH(QString, import_format);
  QFile xml_file(file_path);
  xml_file.open(QIODevice::ReadOnly | QIODevice::Text);
  QXmlStreamReader xml(&xml_file);

  xml.readNextStartElement();
  QCOMPARE(xml.name().toString(), QString("annotated_document_set"));

  for (auto doc : docs) {
    auto json_data = doc.toArray();
    auto meta = json_data[1].toObject();
    while (!xml.readNextStartElement()) {
    }
    QCOMPARE(xml.name().toString(), QString("annotated_document"));
    xml.readNextStartElement();
    QCOMPARE(xml.name().toString(), QString("document_md5_checksum"));
    auto output_md5 = xml.readElementText();
    if (import_format != "txt") {
      QCOMPARE(output_md5, meta.value("original_md5").toString());
    }
    xml.readNextStartElement();
    if (import_with_meta) {
      QCOMPARE(xml.attributes().value("original_md5").toString(),
               meta.value("original_md5").toString());
      QCOMPARE(xml.attributes().value("title").toString(),
               meta.value("title").toString());
    }
    if (export_with_content) {
      QCOMPARE(xml.name().toString(), QString("document"));
      auto text = xml.readElementText();
      if (import_format != "txt") {
        QCOMPARE(text, json_data[0].toString());
      } else {
        QCOMPARE(text, json_data[0].toString().replace("\n", "\t"));
      }
    } else {
      QCOMPARE(xml.name().toString(), QString("meta"));
      QCOMPARE(xml.readElementText(), QString());
    }

    xml.readNextStartElement();
    QCOMPARE(xml.name().toString(), QString("annotation_approver"));
    QCOMPARE(xml.readElementText(), QString("some_user"));

    xml.readNextStartElement();
    QCOMPARE(xml.name().toString(), QString("annotation_set"));
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
      while (!xml.readNextStartElement()) {
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

void TestDatabase::check_exported_docs_json(const QString& file_path,
                                            const QJsonArray& docs) {
  QFETCH(bool, export_with_content);
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
  for (auto doc : docs) {
    auto json_data = doc.toArray();
    auto meta = json_data[1].toObject();
    auto output_json = output_json_data[i].toObject();
    if (import_format != "txt") {
      QCOMPARE(output_json.value("document_md5_checksum").toString(),
               meta.value("original_md5").toString());
    }
    if (import_with_meta) {
      QCOMPARE(
          output_json.value("meta").toObject().value("original_md5").toString(),
          meta.value("original_md5").toString());
      QCOMPARE(
          output_json.value("meta").toObject().value("title").toString(),
          meta.value("title").toString());
    }
    if (export_with_content) {
      if (import_format != "txt") {
        QCOMPARE(output_json.value("text").toString(), json_data[0].toString());
      } else {
        QCOMPARE(output_json.value("text").toString(),
                 json_data[0].toString().replace("\n", "\t"));
      }
    }
    QCOMPARE(output_json.value("annotation_approver").toString(),
             QString("some_user"));

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

    } else if (meta.value("title").toString() == "document 2") {
      auto annotation = output_json.value("labels").toArray()[0].toArray();
      QCOMPARE(annotation[0].toInt(), 3);
      QCOMPARE(annotation[1].toInt(), 4);
      QCOMPARE(annotation[2].toString(),
               QString("label: Reinício da sessão"));
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
  for (auto doc : docs) {
    out << doc.toArray()[0].toString().replace("\n", "\t") << "\n";
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
  for (auto doc : docs) {
    auto doc_array = doc.toArray();
    xml.writeStartElement("document");
    if (import_with_meta) {
      auto extra_data = doc_array[1].toObject();
      for (auto key_val = extra_data.constBegin();
           key_val != extra_data.constEnd(); ++key_val) {
        xml.writeAttribute(key_val.key(),
                           key_val.value().toVariant().toString());
      }
    }
    xml.writeCharacters(doc_array[0].toString());
    xml.writeEndElement();
  }
  xml.writeEndElement();
  xml.writeEndDocument();
}

void TestDatabase::create_documents_file_json(const QString& file_path,
                                              const QJsonArray& docs) {
  QFETCH(QString, import_format);
  QFETCH(bool, import_with_meta);
  QFETCH(bool, as_json_objects);
  auto jsonl{import_format == "jsonl"};
  QFile file(file_path);
  file.open(QIODevice::WriteOnly | QIODevice::Text);
  QTextStream out(&file);
  if (!jsonl) {
    out << "[\n";
  }
  int i{};
  for (auto doc : docs) {
    if (i) {
      if (!jsonl) {
        out << ",";
      }
      out << "\n";
    }
    auto doc_array = doc.toArray();
    if (as_json_objects) {
      QJsonObject json{};
      json.insert("text", doc_array[0].toString());
      if (import_with_meta) {
        json.insert("meta", doc_array[1].toObject());
      }
      out << QString::fromUtf8(
          QJsonDocument(json).toJson(QJsonDocument::Compact));
    } else {
      QJsonArray json_a{};
      json_a << doc_array[0];
      if (import_with_meta) {
        json_a << doc_array[1];
      }
      out << QString::fromUtf8(
          QJsonDocument(json_a).toJson(QJsonDocument::Compact));
    }
    ++i;
  }
  out << (jsonl ? "\n" : "\n]\n");
}

} // namespace labelbuddy
