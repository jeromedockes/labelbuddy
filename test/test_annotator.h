#ifndef LABELBUDDY_TEST_ANNOTATOR_H
#define LABELBUDDY_TEST_ANNOTATOR_H

#include <QTest>

namespace labelbuddy {
class TestAnnotator : public QObject {
  Q_OBJECT
private slots:
  void test_annotator();
  void test_overlapping_annotations();
  void test_extra_data_annotations();
};
} // namespace labelbuddy
#endif
