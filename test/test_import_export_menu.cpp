#include "test_import_export_menu.h"
#include "import_export_menu.h"

#include "database.h"
#include "testing_utils.h"

namespace labelbuddy {

void TestImportExportMenu::test_checkboxes_states() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  DatabaseCatalog catalog{};
  ImportExportMenu menu(&catalog);
  auto checkboxes = menu.findChildren<QCheckBox*>();
  QCOMPARE(checkboxes.size(), 3);
  for (auto cb : checkboxes) {
    QCOMPARE(int(cb->isChecked()), 1);
  }
  catalog.open_database(db_name);
  for (auto cb : checkboxes) {
    QCOMPARE(int(cb->isChecked()), 1);
  }
  catalog.set_app_state_extra("export_include_doc_text", 0);
  catalog.set_app_state_extra("export_labelled_only", 0);
  menu.update_database_info();
  QCOMPARE(int(checkboxes[0]->isChecked()), 0);
  QCOMPARE(int(checkboxes[1]->isChecked()), 0);
  QCOMPARE(int(checkboxes[2]->isChecked()), 1);
}
} // namespace labelbuddy
