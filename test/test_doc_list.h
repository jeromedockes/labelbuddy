#ifndef LABELBUDDY_TEST_DOC_LIST_H
#define LABELBUDDY_TEST_DOC_LIST_H

#include <QTest>

namespace labelbuddy {

class TestDocList : public QObject {
  Q_OBJECT

private slots:

  void test_delete_selected_docs();
  void test_delete_all_docs();
  void test_visit_document();
  void test_navigation();
  void test_filters();
};

} // namespace labelbuddy
#endif
