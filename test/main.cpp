#include <QApplication>
#include <QTest>

#include "test_database.h"
#include "test_searchable_text.h"
#include "test_doc_list_model.h"
#include "test_label_list_model.h"
#include "test_annotations_model.h"
#include "test_annotator.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  int status{};
  status |= QTest::qExec(new labelbuddy::TestSearchableText, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestDatabase, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestDocListModel, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestLabelListModel, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestAnnotationsModel, argc, argv);
  status |= QTest::qExec(new labelbuddy::TestAnnotator, argc, argv);
  return status;
}
