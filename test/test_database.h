#ifndef LABELBUDDY_TEST_DATABASE_H
#define LABELBUDDY_TEST_DATABASE_H

#include <QtTest>

namespace labelbuddy {

class TestDatabase : public QObject {
  Q_OBJECT

private slots:
  void test_open_database();
  void test_import_and_export();
  void test_import_export_xml();
  void test_import_json();
  void test_import_json_lines();
};
} // namespace labelbuddy
#endif
