#ifndef LABELBUDDY_TEST_LABEL_LIST_H
#define LABELBUDDY_TEST_LABEL_LIST_H

#include <QTest>

namespace labelbuddy {
class TestLabelList : public QObject {
  Q_OBJECT
private slots:
  void testLabelList();
};
} // namespace labelbuddy
#endif
