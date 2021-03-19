#ifndef LABELBUDDY_TEST_DATASET_MENU_H
#define LABELBUDDY_TEST_DATASET_MENU_H

#include <QtTest>

#include "dataset_menu.h"

namespace labelbuddy {

class TestDatasetMenu : public QObject {
  Q_OBJECT

private slots:
  void test_dataset_menu();
};
} // namespace labelbuddy

#endif
