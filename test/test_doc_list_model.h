#ifndef LABELBUDDY_TEST_DOC_LIST_MODEL_H
#define LABELBUDDY_TEST_DOC_LIST_MODEL_H

#include <QTest>

namespace labelbuddy {
  class TestDocListModel : public QObject{
    Q_OBJECT
  private slots:
    void test_delete_docs();
    void test_filters();
    void test_updating_results();
  };
}
#endif
