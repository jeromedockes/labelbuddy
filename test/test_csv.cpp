#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTextStream>

#include "csv.h"
#include "test_csv.h"

namespace labelbuddy {

void TestCsv::test_read_write_csv_data() {
  QTest::addColumn<QString>("input_file");
  QTest::addColumn<QString>("input_name");
  QTest::addColumn<bool>("has_error");
  QTest::addColumn<int>("n_rows");
  QTest::addColumn<int>("n_columns");
  QTest::addColumn<QString>("reference_file");
  QTest::addColumn<bool>("map_reader");
  QDir dir(":test/data/example_csv/");
  auto input_files = dir.entryList({"*-in.csv"});
  for (const auto& file_name : input_files) {
    auto file_path = dir.filePath(file_name);
    auto parts = file_name.split("-");
    for (bool map_reader : {false, true}) {
      QTest::newRow(QString("%0, %1").arg(file_name).arg(map_reader).toUtf8())
          << file_path << parts[0] << (parts[1] == "error" ? true : false)
          << parts[2].toInt() << parts[3].toInt()
          << dir.filePath(QString("%0-out.csv").arg(parts[0])) << map_reader;
    }
  }
}

void TestCsv::test_read_write_csv() {
  QFETCH(QString, input_file);
  QFile file(input_file);
  file.open(QIODevice::ReadOnly);
  QTextStream stream(&file);
  QFETCH(bool, map_reader);
  QList<QList<QString>> data{};
  QFETCH(bool, has_error);
  if (map_reader) {
    CsvMapReader reader(&stream);
    data << reader.header();
    while (!reader.at_end()) {
      auto record_map = reader.read_record();
      QList<QString> record{};
      for (const auto& key : reader.header()) {
        if (record_map.contains(key)) {
          record << record_map[key];
        }
      }
      if (!reader.found_error()) {
        data << record;
      }
    }
    QCOMPARE(reader.found_error(), has_error);
  } else {
    CsvReader reader(&stream);
    while (!reader.at_end()) {
      auto record = reader.read_record();
      if (!reader.found_error()) {
        data << record;
      }
    }
    QCOMPARE(reader.found_error(), has_error);
  }
  QFETCH(int, n_rows);
  QCOMPARE(data.size(), n_rows);
  QFETCH(int, n_columns);
  QCOMPARE(data[0].size(), n_columns);

  QTemporaryDir temp_dir{};
  auto out_file_path = temp_dir.filePath("out.csv");
  {
    QFile out_file(out_file_path);
    out_file.open(QIODevice::WriteOnly);
    QTextStream out(&out_file);
    CsvWriter writer(&out);
    for (const auto& record : data) {
      writer.write_record(record.constBegin(), record.constEnd());
    }
  }
  QFETCH(QString, reference_file);
  compare_files(out_file_path, reference_file);
}

void compare_files(const QString& file_a, const QString& file_b) {
  QFile fa(file_a);
  fa.open(QIODevice::ReadOnly);
  auto fa_content = fa.readAll();

  QFile fb(file_b);
  fb.open(QIODevice::ReadOnly);
  auto fb_content = fb.readAll();

  QCOMPARE(fa_content, fb_content);
}
} // namespace labelbuddy
