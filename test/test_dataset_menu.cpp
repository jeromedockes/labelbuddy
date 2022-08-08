#include <QMessageBox>
#include <QTemporaryDir>
#include <QTimer>

#include "doc_list.h"
#include "doc_list_model.h"
#include "label_list_model.h"
#include "test_dataset_menu.h"
#include "testing_utils.h"

namespace labelbuddy {

void TestDatasetMenu::testDatasetMenu() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  addAnnotations(dbName);
  DocListModel docModel{};
  docModel.setDatabase(dbName);
  LabelListModel labelModel{};
  labelModel.setDatabase(dbName);
  DatasetMenu menu{};
  menu.setDocListModel(&docModel);
  menu.setLabelListModel(&labelModel);
  auto docList = menu.findChild<DocList*>();
  auto filterBox = docList->findChildren<QComboBox*>()[0];
  filterBox->setCurrentIndex(1);
  filterBox->activated(1);
  QCOMPARE(docModel.rowCount(), 1);
  QCOMPARE(menu.nSelectedDocs(), 0);
  docList->findChildren<QPushButton*>()[0]->click();
  QCOMPARE(menu.nSelectedDocs(), 1);
  auto lbuttons = menu.findChild<LabelListButtons*>();
  lbuttons->findChildren<QPushButton*>()[0]->click();
  QTimer::singleShot(1000, this, [&]() {
    menu.findChild<QMessageBox*>()->findChildren<QPushButton*>()[0]->click();
  });
  QTimer::singleShot(2000, this, [&]() {
    menu.findChild<QMessageBox*>()->findChildren<QPushButton*>()[0]->click();
  });
  lbuttons->findChildren<QPushButton*>()[1]->click();
  QTest::qWait(1000);
  QCOMPARE(docModel.rowCount(), 0);
}

} // namespace labelbuddy
