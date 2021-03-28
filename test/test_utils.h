#ifndef LABELBUDDY_TEST_UTILS_H
#define LABELBUDDY_TEST_UTILS_H

#include <QTest>

namespace labelbuddy {

class TestUtils : public QObject {
  Q_OBJECT
private slots:
  void test_get_doc_url();
  void test_database_name_display();
};
} // namespace labelbuddy

#endif
