#include "test_import_export_menu.h"
#include "import_export_menu.h"

#include "database.h"
#include "testing_utils.h"

namespace labelbuddy {

void TestImportExportMenu::testCheckboxesStates() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  DatabaseCatalog catalog{};
  ImportExportMenu menu(&catalog);
  auto checkboxes = menu.findChildren<QCheckBox*>();
  QCOMPARE(checkboxes.size(), 3);
  for (auto cb : checkboxes) {
    QCOMPARE(int(cb->isChecked()), 1);
  }
  catalog.openDatabase(dbName);
  for (auto cb : checkboxes) {
    QCOMPARE(int(cb->isChecked()), 1);
  }
  catalog.setAppStateExtra("export_include_doc_text", 0);
  catalog.setAppStateExtra("export_labelled_only", 0);
  menu.updateDatabaseInfo();
  QCOMPARE(int(checkboxes[0]->isChecked()), 0);
  QCOMPARE(int(checkboxes[1]->isChecked()), 0);
  QCOMPARE(int(checkboxes[2]->isChecked()), 1);
}
} // namespace labelbuddy
