#ifndef LABELBUDDY_TEST_LABEL_LIST_MODEL_H
#define LABELBUDDY_TEST_LABEL_LIST_MODEL_H

#include <QTest>

#include "label_list_model.h"

namespace labelbuddy {
class TestLabelListModel : public QObject {
  Q_OBJECT
private slots:
  void test_delete_labels();
  void test_set_shortcut();
  void test_set_color();
  void test_is_valid_shortcut();
  void test_getdata();
  void test_getdata_data();
  void test_add_label();
  void test_mime_drop();
};

QList<QString> get_label_names(const LabelListModel& model);

} // namespace labelbuddy
#endif
