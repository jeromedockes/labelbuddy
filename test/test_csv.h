#ifndef LABELBUDDY_TEST_CSV_H
#define LABELBUDDY_TEST_CSV_H

#include <QTest>

namespace labelbuddy {

class TestCsv : public QObject {
  Q_OBJECT
private slots:
  void test_read_write_csv();
  void test_read_write_csv_data();
};

void compare_files(const QString& file_a, const QString& file_b);
} // namespace labelbuddy

#endif
