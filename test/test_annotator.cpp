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
void TestAnnotator::test_annotator() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  AnnotationsModel annotations_model{};
  annotations_model.set_database(db_name);
  LabelListModel labels_model{};
  labels_model.set_database(db_name);
  Annotator annotator{};
  annotator.set_annotations_model(&annotations_model);
  annotator.set_label_list_model(&labels_model);
  annotations_model.visit_next();
  auto text = annotator.findChild<SearchableText*>();
  QVERIFY(
      text->get_text_edit()->toPlainText().startsWith(QString("document 1")));
  auto nav = annotator.findChild<AnnotationsNavButtons*>();
  auto labels = annotator.findChild<LabelChoices*>();
  auto search = text->findChild<QLineEdit*>();
  auto lv = labels->findChild<QListView*>();
  QVERIFY(!lv->isEnabled());
  search->setText("Friday 17 December 1999");
  text->search_forward();
  QVERIFY(lv->isEnabled());
  auto del = labels->findChild<QPushButton*>();
  QVERIFY(!del->isEnabled());
  // does nothing if no active annotation
  QTest::keyClick(&annotator, Qt::Key_Backspace);
  // set label using label list button
  lv->selectionModel()->select(labels_model.index(1, 0),
                               QItemSelectionModel::SelectCurrent);
  QCOMPARE(annotator.active_annotation_label(), 2);
  QCOMPARE(annotations_model.get_annotations_info()[1].label_id, 2);
  QVERIFY(del->isEnabled());
  // set label using shortcut
  QTest::keyClicks(&annotator, "p");
  QCOMPARE(annotations_model.get_annotations_info()[2].label_id, 1);
  //delete with button
  del->click();
  QCOMPARE(annotations_model.get_annotations_info().size(), 0);

  // create other annotation and delete with backspace
  text->search_forward();
  QTest::keyClicks(&annotator, "p");
  QCOMPARE(annotations_model.get_annotations_info()[2].label_id, 1);
  QTest::keyClick(&annotator, Qt::Key_Backspace);
  QCOMPARE(annotations_model.get_annotations_info().size(), 0);

  nav->findChildren<QPushButton*>()[4]->click();
  QCOMPARE(annotations_model.current_doc_position(), 2);
  search->setText("document 2");
  text->search_forward();
  lv->selectionModel()->select(labels_model.index(1, 0),
                               QItemSelectionModel::SelectCurrent);
  nav->findChildren<QPushButton*>()[2]->click();
  QCOMPARE(annotations_model.current_doc_position(), 1);
  QCOMPARE(annotations_model.get_annotations_info().size(), 0);
  nav->findChildren<QPushButton*>()[2]->click();
  QCOMPARE(annotations_model.current_doc_position(), 0);
  nav->findChildren<QPushButton*>()[5]->click();
  QCOMPARE(annotations_model.current_doc_position(), 2);
  QCOMPARE(annotations_model.get_annotations_info().size(), 1);
  QCOMPARE(annotator.active_annotation_label(), -1);
  annotator.select_next_annotation();
  QCOMPARE(annotator.active_annotation_label(), 2);
}

void TestAnnotator::test_overlapping_annotations() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  AnnotationsModel annotations_model{};
  annotations_model.set_database(db_name);
  LabelListModel labels_model{};
  labels_model.set_database(db_name);
  Annotator annotator{};
  annotator.set_annotations_model(&annotations_model);
  annotator.set_label_list_model(&labels_model);
  annotations_model.visit_next();

  auto lv = annotator.findChild<LabelChoices*>()->findChild<QListView*>();
  auto te = annotator.findChild<SearchableText*>()->get_text_edit();
  auto cursor = te->textCursor();
  cursor.setPosition(0);
  cursor.setPosition(5, QTextCursor::KeepAnchor);
  te->setTextCursor(cursor);

  QTest::keyClicks(&annotator, "p");
  QCOMPARE(te->extraSelections().size(), 1);
  QCOMPARE(annotations_model.get_annotations_info().size(), 1);

  auto status = annotator.current_status_info();
  QCOMPARE(status.annotation_info, QString("^^ 0, 5"));
  QCOMPARE(status.doc_info, QString("1 annotation in current doc"));
  QCOMPARE(status.annotation_label, QString("label: Reinício da sessão"));

  cursor.setPosition(0);
  te->setTextCursor(cursor);
  cursor.setPosition(0);
  cursor.setPosition(5, QTextCursor::KeepAnchor);
  te->setTextCursor(cursor);
  lv->selectionModel()->select(labels_model.index(2, 0),
                               QItemSelectionModel::SelectCurrent);
  QCOMPARE(te->extraSelections().size(), 1);
  QCOMPARE(annotations_model.get_annotations_info().size(), 2);

  cursor.setPosition(0);
  te->setTextCursor(cursor);
  cursor.setPosition(1);
  cursor.setPosition(4, QTextCursor::KeepAnchor);
  te->setTextCursor(cursor);
  lv->selectionModel()->select(labels_model.index(1, 0),
                               QItemSelectionModel::SelectCurrent);
  QCOMPARE(te->extraSelections().size(), 3);
  QCOMPARE(annotations_model.get_annotations_info().size(), 3);

  status = annotator.current_status_info();
  QCOMPARE(status.annotation_info, QString(" 1, 4"));
  QCOMPARE(status.doc_info, QString("3 annotations in current doc"));
  QCOMPARE(status.annotation_label,
           QString("label: Resumption of the session"));
}

void TestAnnotator::test_extra_data_annotations() {
  QTemporaryDir tmp_dir{};
  auto db_name = prepare_db(tmp_dir);
  add_annotations(db_name);
  AnnotationsModel annotations_model{};
  annotations_model.set_database(db_name);
  LabelListModel labels_model{};
  labels_model.set_database(db_name);
  Annotator annotator{};
  annotator.set_annotations_model(&annotations_model);
  annotator.set_label_list_model(&labels_model);

  auto te = annotator.findChild<SearchableText*>()->get_text_edit();
  auto ed = annotator.findChild<LabelChoices*>()->findChild<QLineEdit*>();
  QTest::keyClicks(te, " ");
  QCOMPARE(ed->text(), QString("hello extra data"));
  ed->setText("");
  QTest::keyClicks(ed, "new extra data");
  QSqlQuery query(QSqlDatabase::database(db_name));
  query.exec("select extra_data from annotation where rowid = 1;");
  query.next();
  QCOMPARE(query.value(0).toString(), "new extra data");
  QTest::keyClick(te, Qt::Key_Escape);
  QCOMPARE(ed->text(), QString(""));
}
} // namespace labelbuddy
