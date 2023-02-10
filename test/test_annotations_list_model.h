#ifndef LABELBUDDY_TEST_ANNOTATIONS_LIST_MODEL_H
#define LABELBUDDY_TEST_ANNOTATIONS_LIST_MODEL_H

#include <QObject>
#include <QTest>

namespace labelbuddy {

class TestAnnotationsListModel : public QObject {
  Q_OBJECT

private slots:
  void testAddAndDeleteAnnotations();
  void testRoles();
};
} // namespace labelbuddy

#endif
