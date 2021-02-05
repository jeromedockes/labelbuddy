#ifndef LABELBUDDY_TEST_LABEL_LIST_MODEL_H
#define LABELBUDDY_TEST_LABEL_LIST_MODEL_H

#include <QTest>

namespace labelbuddy {
class TestLabelListModel : public QObject {
  Q_OBJECT
private slots:
  void test_delete_labels();
};
} // namespace labelbuddy
#endif
