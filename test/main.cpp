#include <QApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "test_annotations_model.h"
#include "test_annotator.h"
#include "test_char_indices.h"
#include "test_database.h"
#include "test_dataset_menu.h"
#include "test_doc_list.h"
#include "test_doc_list_model.h"
#include "test_import_export_menu.h"
#include "test_label_list.h"
#include "test_label_list_model.h"
#include "test_main_window.h"
#include "test_searchable_text.h"
#include "test_utils.h"
#include "test_annotations_list_model.h"
#include "test_annotations_list.h"

int main(int argc, char* argv[]) {
  QTemporaryDir tmpDir{};
  QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                     tmpDir.path());

  QApplication app(argc, argv);

  int status{};
  status |= QTest::qExec(new labelbuddy::TestSearchableText, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestDatabase, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestDocListModel, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestLabelListModel, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestAnnotationsModel, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestAnnotator, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestDatasetMenu, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestDocList, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestLabelBuddy, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestUtils, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestLabelList, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestImportExportMenu, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestCharIndices, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestAnnotationsListModel, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestAnnotationsList, argc, argv);
  return status;
}
