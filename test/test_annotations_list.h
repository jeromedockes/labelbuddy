#ifndef LABELBUDDY_TEST_ANNOTATIONS_LIST_H
#define LABELBUDDY_TEST_ANNOTATIONS_LIST_H

#include <QObject>
#include <QTest>

namespace labelbuddy {

  class TestAnnotationsList : public QObject {
    Q_OBJECT

  private slots:
    void testAnnotationsList();
  };
}

#endif
