#ifndef LABELBUDDY_TEST_ANNOTATIONS_MODEL_H
#define LABELBUDDY_TEST_ANNOTATIONS_MODEL_H

#include <QTest>

namespace labelbuddy {
class TestAnnotationsModel : public QObject {
  Q_OBJECT
private slots:
  void testAddAndDeleteAnnotations();
  void testNavigation();
  void testSurrogatePairs();
};
} // namespace labelbuddy
#endif
