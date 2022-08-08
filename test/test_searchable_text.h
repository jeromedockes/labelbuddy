#ifndef LABELBUDDY_TEST_SEARCHABLE_TEXT_H
#define LABELBUDDY_TEST_SEARCHABLE_TEXT_H

#include <QtTest>

namespace labelbuddy {

class TestSearchableText : public QObject {
  Q_OBJECT

private slots:
  void testSearch();
  void testCyclePos();
  void testShortcuts();
};

} // namespace labelbuddy

#endif
