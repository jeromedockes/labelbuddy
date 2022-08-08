#include <QComboBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>

#include "database.h"
#include "doc_list.h"
#include "test_doc_list.h"
#include "testing_utils.h"

namespace labelbuddy {

void TestDocList::testDeleteAllDocs() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  addAnnotations(dbName);
  DocListModel docModel{};
  docModel.setDatabase(dbName);
  DocList docList{};
  docList.setModel(&docModel);

  QCOMPARE(docModel.totalNDocs(), 6);
  QTimer::singleShot(1000, this, [&]() {
    docList.findChild<QMessageBox*>()->findChildren<QPushButton*>()[0]->click();
  });
  QTimer::singleShot(2000, this, [&]() {
    docList.findChild<QMessageBox*>()->findChildren<QPushButton*>()[0]->click();
  });
  docList.findChild<DocListButtons*>()
      ->findChildren<QPushButton*>()[2]
      ->click();
  QCOMPARE(docModel.totalNDocs(), 0);
}

void TestDocList::testDeleteSelectedDocs() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  addAnnotations(dbName);
  DocListModel docModel{};
  docModel.setDatabase(dbName);
  DocList docList{};
  docList.setModel(&docModel);

  QSignalSpy spy(&docList, SIGNAL(nSelectedDocsChanged(int)));

  QCOMPARE(docModel.totalNDocs(), 6);
  QCOMPARE(docList.nSelectedDocs(), 0);

  auto lv = docList.findChild<QListView*>();
  lv->selectionModel()->select(docModel.index(0, 0),
                               QItemSelectionModel::Select);
  lv->selectionModel()->select(docModel.index(2, 0),
                               QItemSelectionModel::Select);
  QCOMPARE(spy.count(), 2);
  QVERIFY(spy[1].at(0).toInt() == 2);
  QCOMPARE(docList.nSelectedDocs(), 2);
  QTimer::singleShot(1000, this, [&]() {
    docList.findChild<QMessageBox*>()->findChildren<QPushButton*>()[0]->click();
  });
  QTimer::singleShot(2000, this, [&]() {
    docList.findChild<QMessageBox*>()->findChildren<QPushButton*>()[0]->click();
  });
  docList.findChild<DocListButtons*>()
      ->findChildren<QPushButton*>()[1]
      ->click();
  QCOMPARE(docModel.totalNDocs(), 4);
  QCOMPARE(docList.nSelectedDocs(), 0);
  QCOMPARE(spy.count(), 3);
  QVERIFY(spy[2].at(0).toInt() == 0);

  lv->selectAll();
  QCOMPARE(docList.nSelectedDocs(), 4);
  QCOMPARE(spy.count(), 4);
  QVERIFY(spy[3].at(0).toInt() == 4);

  QTimer::singleShot(1000, this, [&]() {
    docList.findChild<QMessageBox*>()->findChildren<QPushButton*>()[0]->click();
  });
  QTimer::singleShot(2000, this, [&]() {
    docList.findChild<QMessageBox*>()->findChildren<QPushButton*>()[0]->click();
  });
  docList.findChild<DocListButtons*>()
      ->findChildren<QPushButton*>()[1]
      ->click();
  QCOMPARE(docModel.totalNDocs(), 0);
  QCOMPARE(docList.nSelectedDocs(), 0);
  QCOMPARE(spy.count(), 5);
  QVERIFY(spy[4].at(0).toInt() == 0);
}

void TestDocList::testVisitDocument() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  addAnnotations(dbName);
  DocListModel docModel{};
  docModel.setDatabase(dbName);
  DocList docList{};
  docList.setModel(&docModel);

  QSignalSpy spy(&docList, SIGNAL(visitDocRequested(int)));

  docList.findChild<DocListButtons*>()
      ->findChildren<QPushButton*>()[3]
      ->click();

  QCOMPARE(spy.count(), 0);

  auto lv = docList.findChild<QListView*>();
  lv->selectionModel()->select(docModel.index(0, 0),
                               QItemSelectionModel::Select);

  docList.findChild<DocListButtons*>()
      ->findChildren<QPushButton*>()[3]
      ->click();

  QCOMPARE(spy.count(), 1);
  QVERIFY(spy[0].at(0).toInt() == 1);
}

void TestDocList::testNavigation() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  addAnnotations(dbName);
  addManyDocs(dbName);
  DocListModel docModel{};
  docModel.setDatabase(dbName);
  DocList docList{};
  docList.setModel(&docModel);
  QCOMPARE(docModel.totalNDocs(), 366);

  auto filterBox = docList.findChild<QComboBox*>();
  auto buttons = docList.findChildren<QPushButton*>();
  auto first = buttons[4];
  auto prev = buttons[5];
  auto next = buttons[6];
  auto last = buttons[7];
  QLabel* label = docList.findChildren<QLabel*>().back();

  QVERIFY(!first->isEnabled());
  QVERIFY(!prev->isEnabled());
  QVERIFY(next->isEnabled());
  QVERIFY(last->isEnabled());

  next->click();
  QCOMPARE(label->text(), QString("101 - 200 / 366"));

  next->click();
  QCOMPARE(label->text(), QString("201 - 300 / 366"));

  last->click();
  QCOMPARE(label->text(), QString("301 - 366 / 366"));
  QVERIFY(!next->isEnabled());
  QVERIFY(!last->isEnabled());

  prev->click();
  QCOMPARE(label->text(), QString("201 - 300 / 366"));

  first->click();
  QCOMPARE(label->text(), QString("1 - 100 / 366"));

  last->click();
  QCOMPARE(label->text(), QString("301 - 366 / 366"));

  filterBox->setCurrentIndex(1);
  filterBox->activated(1);
  QCOMPARE(label->text(), QString("1 / 1"));
  QVERIFY(!first->isEnabled());
  QVERIFY(!prev->isEnabled());
  QVERIFY(!next->isEnabled());
  QVERIFY(!last->isEnabled());

  filterBox->setCurrentIndex(2);
  filterBox->activated(2);
  QCOMPARE(label->text(), QString("1 - 100 / 365"));

  filterBox->setCurrentIndex(0);
  filterBox->activated(0);
  QCOMPARE(label->text(), QString("1 - 100 / 366"));
}

void TestDocList::testFilters() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  addAnnotations(dbName);
  DocListModel docModel{};
  docModel.setDatabase(dbName);
  DocList docList{};
  docList.setModel(&docModel);
  QCOMPARE(docModel.rowCount(), 6);

  auto filterBox = docList.findChild<QComboBox*>();

  // separators count as items
  QCOMPARE(filterBox->count(), 11);

  filterBox->setCurrentIndex(1);
  filterBox->activated(1);
  QCOMPARE(docModel.rowCount(), 1);
  docModel.labelsChanged();
  QCOMPARE(filterBox->currentIndex(), 1);
  QCOMPARE(docModel.rowCount(), 1);

  filterBox->setCurrentIndex(2);
  filterBox->activated(2);
  QCOMPARE(docModel.rowCount(), 5);
  docModel.labelsChanged();
  QCOMPARE(filterBox->currentIndex(), 2);
  QCOMPARE(docModel.rowCount(), 5);

  // label 1
  filterBox->setCurrentIndex(4);
  filterBox->activated(4);
  auto labelId = filterBox->currentData().value<QPair<int, int>>().first;
  QCOMPARE(labelId, 1);
  QCOMPARE(docModel.rowCount(), 1);

  filterBox->setCurrentIndex(5);
  filterBox->activated(5);
  QCOMPARE(filterBox->currentText(),
           QString("label: Resumption of the session"));
  QCOMPARE(docModel.rowCount(), 0);

  // not label 1
  filterBox->setCurrentIndex(8);
  filterBox->activated(8);
  QCOMPARE(docModel.rowCount(), 5);

  filterBox->setCurrentIndex(10);
  filterBox->activated(10);
  QCOMPARE(docModel.rowCount(), 6);

  // not label 1
  filterBox->setCurrentIndex(8);
  filterBox->activated(8);
  QCOMPARE(docModel.rowCount(), 5);
  QSqlQuery query(QSqlDatabase::database(dbName));
  query.exec("delete from label where id = 2;");
  docModel.labelsChanged();
  QCOMPARE(filterBox->currentIndex(), 7);
  QCOMPARE(docModel.rowCount(), 5);
  query.exec("delete from annotation;");
  docModel.documentLostLabel(1, 1);
  docList.show();
  QCOMPARE(docModel.rowCount(), 6);

  auto newDb = tmpDir.filePath("db1");
  { DatabaseCatalog().openDatabase(newDb); }
  docModel.setDatabase(newDb);
  // separators have been removed
  QCOMPARE(filterBox->count(), 3);
  QCOMPARE(filterBox->currentIndex(), 0);
}
} // namespace labelbuddy
