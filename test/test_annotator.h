#ifndef LABELBUDDY_TEST_ANNOTATOR_H
#define LABELBUDDY_TEST_ANNOTATOR_H

#include <QTest>

namespace labelbuddy {
class TestAnnotator : public QObject {
  Q_OBJECT
private slots:
  void testAnnotator();
  void testOverlappingAnnotations();
  void testExtraDataAnnotations();
};
} // namespace labelbuddy
#endif
