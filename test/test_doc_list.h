#ifndef LABELBUDDY_TEST_DOC_LIST_H
#define LABELBUDDY_TEST_DOC_LIST_H

#include <QTest>

namespace labelbuddy {

class TestDocList : public QObject {
  Q_OBJECT

private slots:

  void testDeleteSelectedDocs();
  void testDeleteAllDocs();
  void testVisitDocument();
  void testNavigation();
  void testFilters();
};

} // namespace labelbuddy
#endif
