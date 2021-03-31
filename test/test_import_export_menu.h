#ifndef LABELBUDDY_TEST_IMPORT_EXPORT_MENU_H
#define LABELBUDDY_TEST_IMPORT_EXPORT_MENU_H

#include <QtTest>

namespace labelbuddy {

class TestImportExportMenu : public QObject {
  Q_OBJECT

private slots:

  void test_checkboxes_states();
};
} // namespace labelbuddy

#endif
