#ifndef LABELBUDDY_TEST_LABEL_LIST_MODEL_H
#define LABELBUDDY_TEST_LABEL_LIST_MODEL_H

#include <QTest>

#include "label_list_model.h"

namespace labelbuddy {
class TestLabelListModel : public QObject {
  Q_OBJECT
private slots:
  void testDeleteLabels();
  void testSetShortcut();
  void testSetColor();
  void testIsValidShortcut();
  void testGetData();
  void testGetData_data();
  void testAddLabel();
  void testMimeDrop();
};

QList<QString> getLabelNames(const LabelListModel& model);

} // namespace labelbuddy
#endif
