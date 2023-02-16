#include <QItemSelectionModel>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "annotations_model.h"
#include "annotator.h"
#include "label_list_model.h"
#include "test_annotator.h"
#include "testing_utils.h"

namespace labelbuddy {
void TestAnnotator::testAnnotator() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  AnnotationsModel annotationsModel{};
  annotationsModel.setDatabase(dbName);
  LabelListModel labelsModel{};
  labelsModel.setDatabase(dbName);
  Annotator annotator{};
  annotator.setAnnotationsModel(&annotationsModel);
  annotator.setLabelListModel(&labelsModel);
  annotationsModel.visitNext();
  auto text = annotator.findChild<SearchableText*>();
  QVERIFY(text->getTextEdit()->toPlainText().startsWith(QString("document 1")));
  auto nav = annotator.findChild<AnnotationsNavButtons*>();
  auto labels = annotator.findChild<AnnotationEditor*>();
  auto search = text->findChild<QLineEdit*>();
  auto lv = labels->findChild<QListView*>();
  QVERIFY(!lv->isEnabled());
  search->setText("Friday 17 December 1999");
  text->searchForward();
  QVERIFY(lv->isEnabled());
  auto del = labels->findChild<QPushButton*>();
  QVERIFY(!del->isEnabled());
  // does nothing if no active annotation
  QTest::keyClick(&annotator, Qt::Key_Backspace);
  // set label using label list button
  lv->selectionModel()->select(labelsModel.index(1, 0),
                               QItemSelectionModel::SelectCurrent);
  QCOMPARE(annotator.activeAnnotationLabel(), 2);
  QCOMPARE(annotationsModel.getAnnotationsInfo()[1].labelId, 2);
  QVERIFY(del->isEnabled());
  // set label using shortcut
  QTest::keyClicks(&annotator, "p");
  QCOMPARE(annotationsModel.getAnnotationsInfo()[2].labelId, 1);
  // delete with button
  del->click();
  QCOMPARE(annotationsModel.getAnnotationsInfo().size(), 1);

  // create other annotation and delete with backspace
  text->searchForward();
  QTest::keyClicks(&annotator, "p");
  QCOMPARE(annotationsModel.getAnnotationsInfo()[2].labelId, 1);
  QTest::keyClick(&annotator, Qt::Key_Backspace);
  QCOMPARE(annotationsModel.getAnnotationsInfo().size(), 1);
  // delete the remaining annotation
  QTest::keyClick(&annotator, Qt::Key_Space);
  QTest::keyClick(&annotator, Qt::Key_Backspace);
  QCOMPARE(annotationsModel.getAnnotationsInfo().size(), 0);

  nav->findChildren<QPushButton*>()[4]->click();
  QCOMPARE(annotationsModel.currentDocPosition(), 2);
  search->setText("document 2");
  text->searchForward();
  lv->selectionModel()->select(labelsModel.index(1, 0),
                               QItemSelectionModel::SelectCurrent);
  nav->findChildren<QPushButton*>()[2]->click();
  QCOMPARE(annotationsModel.currentDocPosition(), 1);
  QCOMPARE(annotationsModel.getAnnotationsInfo().size(), 0);
  nav->findChildren<QPushButton*>()[2]->click();
  QCOMPARE(annotationsModel.currentDocPosition(), 0);
  nav->findChildren<QPushButton*>()[5]->click();
  QCOMPARE(annotationsModel.currentDocPosition(), 2);
  QCOMPARE(annotationsModel.getAnnotationsInfo().size(), 1);
  QCOMPARE(annotator.activeAnnotationLabel(), -1);
  annotator.selectNextAnnotation();
  QCOMPARE(annotator.activeAnnotationLabel(), 2);
}

void TestAnnotator::testOverlappingAnnotations() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  AnnotationsModel annotationsModel{};
  annotationsModel.setDatabase(dbName);
  LabelListModel labelsModel{};
  labelsModel.setDatabase(dbName);
  Annotator annotator{};
  annotator.setAnnotationsModel(&annotationsModel);
  annotator.setLabelListModel(&labelsModel);
  annotationsModel.visitNext();

  auto lv = annotator.findChild<AnnotationEditor*>()->findChild<QListView*>();
  auto te = annotator.findChild<SearchableText*>()->getTextEdit();
  auto cursor = te->textCursor();
  cursor.setPosition(0);
  cursor.setPosition(5, QTextCursor::KeepAnchor);
  te->setTextCursor(cursor);

  QTest::keyClicks(&annotator, "p");
  QCOMPARE(te->extraSelections().size(), 1);
  QCOMPARE(annotationsModel.getAnnotationsInfo().size(), 1);

  auto status = annotator.currentStatusInfo();
  QCOMPARE(status.annotationInfo, QString("^^ 0, 5"));
  QCOMPARE(status.docInfo, QString("1 annotation in current doc"));
  QCOMPARE(status.annotationLabel, QString("label: Reinício da sessão"));

  cursor.setPosition(0);
  te->setTextCursor(cursor);
  cursor.setPosition(0);
  cursor.setPosition(5, QTextCursor::KeepAnchor);
  te->setTextCursor(cursor);
  lv->selectionModel()->select(labelsModel.index(2, 0),
                               QItemSelectionModel::SelectCurrent);
  QCOMPARE(te->extraSelections().size(), 1);
  QCOMPARE(annotationsModel.getAnnotationsInfo().size(), 2);

  // selecting a label with an annotation that already exists at that position
  lv->selectionModel()->select(labelsModel.index(0, 0),
                               QItemSelectionModel::SelectCurrent);
  QCOMPARE(status.annotationLabel, QString("label: Reinício da sessão"));
  QCOMPARE(annotationsModel.getAnnotationsInfo().size(), 2);

  cursor.setPosition(0);
  te->setTextCursor(cursor);
  cursor.setPosition(1);
  cursor.setPosition(4, QTextCursor::KeepAnchor);
  te->setTextCursor(cursor);
  lv->selectionModel()->select(labelsModel.index(1, 0),
                               QItemSelectionModel::SelectCurrent);
  QCOMPARE(te->extraSelections().size(), 3);
  QCOMPARE(annotationsModel.getAnnotationsInfo().size(), 3);

  status = annotator.currentStatusInfo();
  QCOMPARE(status.annotationInfo, QString(" 1, 4"));
  QCOMPARE(status.docInfo, QString("3 annotations in current doc"));
  QCOMPARE(status.annotationLabel, QString("label: Resumption of the session"));
}

void TestAnnotator::testExtraDataAnnotations() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  addAnnotations(dbName);
  AnnotationsModel annotationsModel{};
  annotationsModel.setDatabase(dbName);
  LabelListModel labelsModel{};
  labelsModel.setDatabase(dbName);
  Annotator annotator{};
  annotator.setAnnotationsModel(&annotationsModel);
  annotator.setLabelListModel(&labelsModel);

  auto te = annotator.findChild<SearchableText*>()->getTextEdit();
  auto ed = annotator.findChild<AnnotationEditor*>()->findChild<QLineEdit*>();
  auto lv = annotator.findChild<AnnotationEditor*>()->findChild<QListView*>();
  QTest::keyClicks(te, " ");
  QCOMPARE(ed->text(), QString("hello extra data"));
  ed->setText("");
  QTest::keyClicks(ed, "new extra data");
  QSqlQuery query(QSqlDatabase::database(dbName));
  query.exec("select extra_data from annotation where rowid = 1;");
  query.next();
  QCOMPARE(query.value(0).toString(), "new extra data");
  QTest::keyClick(te, Qt::Key_Escape);
  QCOMPARE(ed->text(), QString(""));

  auto cursor = te->textCursor();
  cursor.setPosition(0);
  cursor.setPosition(5, QTextCursor::KeepAnchor);
  te->setTextCursor(cursor);
  QTest::keyClicks(&annotator, "p");
  QTest::keyClicks(ed, "n");
  QCOMPARE(ed->completer()->model()->rowCount(), 1);
  QCOMPARE(ed->completer()->currentIndex().data(), "new extra data");

  lv->selectionModel()->select(labelsModel.index(2, 0),
                               QItemSelectionModel::SelectCurrent);
  QTest::keyClicks(ed, "n");
  QCOMPARE(ed->completer()->model()->rowCount(), 0);
}
} // namespace labelbuddy
