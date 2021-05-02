#ifndef LABELBUDDY_TEST_ANNOTATIONS_MODEL_H
#define LABELBUDDY_TEST_ANNOTATIONS_MODEL_H

#include <QTest>

namespace labelbuddy {
  class TestAnnotationsModel : public QObject{
    Q_OBJECT
  private slots:
    void test_add_and_delete_annotations();
    void test_navigation();
    void test_surrogate_pairs();
  };
}
#endif
