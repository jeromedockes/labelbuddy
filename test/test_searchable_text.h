#ifndef LABELBUDDY_TEST_SEARCHABLE_TEXT_H
#define LABELBUDDY_TEST_SEARCHABLE_TEXT_H

#include <QtTest>

namespace labelbuddy {

class TestSearchableText : public QObject {
  Q_OBJECT

private slots:
  void test_search();
};

} // namespace labelbuddy

#endif
