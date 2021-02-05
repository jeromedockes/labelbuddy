#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QItemSelectionModel>

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
  QCOMPARE(text->get_text_edit()->toPlainText(),
           QString("TWO this is another, different document"));
  auto nav = annotator.findChild<AnnotationsNavButtons*>();
  auto labels = annotator.findChild<LabelChoices*>();
  auto search = text->findChild<QLineEdit*>();
  auto lv = labels->findChild<QListView*>();
  QVERIFY(!lv->isEnabled());
  search->setText(", different");
  text->search_forward();
  QVERIFY(lv->isEnabled());
  auto del = labels->findChild<QPushButton*>();
  QVERIFY(!del->isEnabled());
  lv->selectionModel()->select(labels_model.index(1, 0),
                               QItemSelectionModel::SelectCurrent);
  QCOMPARE(annotator.active_annotation_label(), 2);
  QCOMPARE(annotations_model.get_annotations_info()[1].label_id, 2);
  QVERIFY(del->isEnabled());
  lv->selectionModel()->select(labels_model.index(0, 0),
                               QItemSelectionModel::SelectCurrent);
  QCOMPARE(annotations_model.get_annotations_info()[1].label_id, 1);
  del->click();
  QCOMPARE(annotations_model.get_annotations_info().size(), 0);
  nav->findChildren<QPushButton*>()[4]->click();
  QCOMPARE(annotations_model.current_doc_position(), 2);
  search->setText("THREE");
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
}
} // namespace labelbuddy
