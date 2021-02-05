#include <QColor>
#include <QFont>
#include <QItemSelectionModel>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QSqlQueryModel>
#include <QVBoxLayout>
#include <QWidget>

#include "annotator.h"
#include "label_list.h"
#include "searchable_text.h"
#include "user_roles.h"

namespace labelbuddy {

LabelChoices::LabelChoices(QWidget* parent) : QWidget(parent) {
  QVBoxLayout* layout = new QVBoxLayout();
  setLayout(layout);
  delete_button = new QPushButton("Remove");
  layout->addWidget(delete_button);
  this->setStyleSheet("QListView::item {background: transparent;}");
  labels_view = new NoDeselectAllView();
  layout->addWidget(labels_view);
  labels_view->setSpacing(3);
  labels_view->setFocusPolicy(Qt::NoFocus);
  labels_view->setItemDelegate(new LabelDelegate);

  QObject::connect(delete_button, &QPushButton::clicked, this,
                   &LabelChoices::on_delete_button_click);
}

void LabelChoices::setModel(LabelListModel* new_model) {
  label_list_model = new_model;
  labels_view->setModel(new_model);
  QObject::connect(labels_view->selectionModel(),
                   &QItemSelectionModel::selectionChanged, this,
                   &LabelChoices::on_selection_change);
}

void LabelChoices::on_selection_change() { emit selectionChanged(); }
void LabelChoices::on_delete_button_click() { emit delete_button_clicked(); }

QModelIndexList LabelChoices::selectedIndexes() const {
  return labels_view->selectionModel()->selectedIndexes();
}

int LabelChoices::selected_label_id() const {
  if (label_list_model == nullptr) {
    return -1;
  }
  QModelIndexList selected_indexes =
      labels_view->selectionModel()->selectedIndexes();
  if (selected_indexes.length() == 0) {
    return -1;
  }
  int label_id =
      label_list_model->data(selected_indexes[0], Roles::RowIdRole).toInt();
  return label_id;
}

void LabelChoices::set_selected_label_id(int label_id) {
  if (label_list_model == nullptr) {
    return;
  }
  if (label_id == -1) {
    labels_view->clearSelection();
    return;
  }
  auto model_index = label_list_model->label_id_to_model_index(label_id);
  if (!model_index.isValid()) {
    return;
  }
  labels_view->setCurrentIndex(model_index);
}

void LabelChoices::enable_delete() { delete_button->setEnabled(true); }
void LabelChoices::disable_delete() { delete_button->setDisabled(true); }
void LabelChoices::enable_label_choice() { labels_view->setEnabled(true); }
void LabelChoices::disable_label_choice() { labels_view->setDisabled(true); }

Annotator::Annotator(QWidget* parent) : QSplitter(parent) {
  label_choices = new LabelChoices();
  addWidget(label_choices);
  auto text_frame = new QFrame();
  addWidget(text_frame);
  auto text_layout = new QVBoxLayout();
  text_frame->setLayout(text_layout);
  nav_buttons = new AnnotationsNavButtons();
  text_layout->addWidget(nav_buttons);
  text = new SearchableText();
  text_layout->addWidget(text);
  default_weight = text->get_text_edit()->fontWeight();
  text->fill("");
  text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  QSettings settings("labelbuddy", "labelbuddy");
  if (settings.contains("Annotator/state")) {
    restoreState(settings.value("Annotator/state").toByteArray());
  }

  QObject::connect(label_choices, &LabelChoices::selectionChanged, this,
                   &Annotator::set_label_for_selected_region);
  QObject::connect(label_choices, &LabelChoices::delete_button_clicked, this,
                   &Annotator::delete_active_annotation);
  QObject::connect(this, &Annotator::active_annotation_changed, this,
                   &Annotator::paint_annotations);
  QObject::connect(this, &Annotator::active_annotation_changed, this,
                   &Annotator::update_label_choices_button_states);
  QObject::connect(text->get_text_edit(), &QTextEdit::selectionChanged, this,
                   &Annotator::update_label_choices_button_states);
  QObject::connect(text->get_text_edit(), &QTextEdit::cursorPositionChanged,
                   this, &Annotator::activate_annotation_at_cursor_pos);
}

void Annotator::store_state() {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("Annotator/state", saveState());
}

void Annotator::set_annotations_model(AnnotationsModel* new_model) {
  annotations_model = new_model;
  nav_buttons->setModel(new_model);

  QObject::connect(annotations_model, &AnnotationsModel::document_changed, this,
                   &Annotator::reset_document);

  QObject::connect(nav_buttons, &AnnotationsNavButtons::visit_next,
                   annotations_model, &AnnotationsModel::visit_next);
  QObject::connect(nav_buttons, &AnnotationsNavButtons::visit_prev,
                   annotations_model, &AnnotationsModel::visit_prev);
  QObject::connect(nav_buttons, &AnnotationsNavButtons::visit_next_labelled,
                   annotations_model, &AnnotationsModel::visit_next_labelled);
  QObject::connect(nav_buttons, &AnnotationsNavButtons::visit_prev_labelled,
                   annotations_model, &AnnotationsModel::visit_prev_labelled);
  QObject::connect(nav_buttons, &AnnotationsNavButtons::visit_next_unlabelled,
                   annotations_model, &AnnotationsModel::visit_next_unlabelled);
  QObject::connect(nav_buttons, &AnnotationsNavButtons::visit_prev_unlabelled,
                   annotations_model, &AnnotationsModel::visit_prev_unlabelled);

  reset_document();
}

void Annotator::reset_document() {
  deactivate_active_annotation();
  text->fill(annotations_model->get_content());
  update_annotations();
}

void Annotator::update_annotations() {
  fetch_labels_info();
  fetch_annotations_info();
  update_label_choices_button_states();
  paint_annotations();
}

void Annotator::set_label_list_model(LabelListModel* new_model) {
  label_choices->setModel(new_model);
}

void Annotator::update_label_choices_button_states() {
  label_choices->set_selected_label_id(active_annotation_label());
  if (active_annotation != -1) {
    label_choices->enable_delete();
    label_choices->enable_label_choice();
    return;
  }
  label_choices->disable_delete();
  auto selection = text->current_selection();
  if (selection[0] != selection[1]) {
    label_choices->enable_label_choice();
  } else {
    label_choices->disable_label_choice();
  }
}

int Annotator::annotation_at_pos(int char_pos) const {
  for (auto anno : annotations) {
    if (anno.start_char <= char_pos && anno.end_char > char_pos) {
      return anno.id;
    }
  }
  return -1;
}

void Annotator::update_nav_buttons() { nav_buttons->update_button_states(); }

int Annotator::active_annotation_label() const {
  if (active_annotation == -1) {
    return -1;
  }
  return annotations[active_annotation].label_id;
}

void Annotator::deactivate_active_annotation() {
  if (!annotations.contains(active_annotation)) {
    return;
  }
  auto fmt = annotations[active_annotation].cursor.charFormat();
  fmt.setFontWeight(default_weight);
  annotations[active_annotation].cursor.setCharFormat(fmt);
  active_annotation = -1;
}

void Annotator::activate_annotation_at_cursor_pos() {
  auto cursor = text->get_text_edit()->textCursor();
  // only activate if click, not drag
  if (cursor.anchor() != cursor.position()) {
    if (active_annotation != -1) {
      deactivate_active_annotation();
      emit active_annotation_changed();
    }
    return;
  }
  auto annotation_id = annotation_at_pos(cursor.position());
  if (active_annotation == annotation_id) {
    return;
  }
  deactivate_active_annotation();
  active_annotation = annotation_id;
  emit active_annotation_changed();
}

void Annotator::delete_annotations(QList<int> annotation_ids) {
  QList<int> to_delete{};
  for (auto id : annotation_ids) {
    if (active_annotation == id) {
      deactivate_active_annotation();
    }
    annotations.remove(id);
    to_delete << id;
  }
  if (!to_delete.length()) {
    return;
  }
  annotations_model->delete_annotations(to_delete);
  emit active_annotation_changed();
}

void Annotator::delete_annotation(int annotation_id) {
  delete_annotations(QList<int>{annotation_id});
}

void Annotator::delete_overlapping_annotations(int start_char, int end_char) {
  QList<int> to_delete{};
  for (auto anno : annotations) {
    if (start_char < anno.end_char && anno.start_char < end_char) {
      to_delete << anno.id;
    }
  }
  delete_annotations(to_delete);
}

void Annotator::delete_active_annotation() {
  delete_annotation(active_annotation);
}

void Annotator::set_label_for_selected_region() {
  if (active_annotation == -1) {
    add_annotation();
    return;
  }
  int label_id = label_choices->selected_label_id();
  auto prev_label = annotations[active_annotation].label_id;
  if (label_id == prev_label) {
    return;
  }
  auto start = annotations[active_annotation].start_char;
  auto end = annotations[active_annotation].end_char;
  delete_annotation(active_annotation);
  add_annotation(label_id, start, end);
}

void Annotator::add_annotation() {
  int label_id = label_choices->selected_label_id();
  if (label_id == -1) {
    return;
  }
  QList<int> selection_boundaries = text->current_selection();
  auto start = selection_boundaries[0];
  auto end = selection_boundaries[1];
  if (start == end) {
    return;
  }
  add_annotation(label_id, start, end);
}

void Annotator::add_annotation(int label_id, int start_char, int end_char) {
  auto annotation_id =
      annotations_model->add_annotation(label_id, start_char, end_char);
  delete_overlapping_annotations(start_char, end_char);
  auto annotation_cursor = text->get_text_edit()->textCursor();
  annotation_cursor.setPosition(start_char);
  annotation_cursor.setPosition(end_char, QTextCursor::KeepAnchor);
  annotations[annotation_id] = AnnotationCursor{
      annotation_id, label_id, start_char, end_char, annotation_cursor};
  auto new_cursor = text->get_text_edit()->textCursor();
  new_cursor.clearSelection();
  text->get_text_edit()->setTextCursor(new_cursor);
  deactivate_active_annotation();
  active_annotation = annotation_id;
  emit active_annotation_changed();
}

void Annotator::fetch_labels_info() {
  if (annotations_model == nullptr) {
    return;
  }
  labels = annotations_model->get_labels_info();
}

void Annotator::fetch_annotations_info() {
  if (annotations_model == nullptr) {
    return;
  }
  int prev_active{active_annotation};
  deactivate_active_annotation();
  auto annotation_positions = annotations_model->get_annotations_info();
  annotations = QMap<int, AnnotationCursor>{};
  for (auto i = annotation_positions.constBegin();
       i != annotation_positions.constEnd(); ++i) {
    auto start = i.value().start_char;
    auto end = i.value().end_char;
    auto cursor = text->get_text_edit()->textCursor();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    annotations[i.value().id] =
        AnnotationCursor{i.value().id, i.value().label_id, start, end, cursor};
  }
  if (annotations.contains(prev_active)) {
    active_annotation = prev_active;
  }
}

void Annotator::paint_annotations() {
  QList<QTextEdit::ExtraSelection> new_selections{};
  for (auto anno : annotations) {
    QTextCharFormat format(anno.cursor.charFormat());
    QColor color(labels.value(anno.label_id).color);
    format.setBackground(color);
    if (anno.id == active_annotation) {
      format.setFontUnderline(true);
      auto fmt = anno.cursor.charFormat();
      fmt.setFontWeight(QFont::DemiBold);
      anno.cursor.setCharFormat(fmt);
    }
    new_selections << QTextEdit::ExtraSelection{anno.cursor, format};
  }
  text->get_text_edit()->setExtraSelections(new_selections);
}

AnnotationsNavButtons::AnnotationsNavButtons(QWidget* parent)
    : QWidget(parent) {
  auto layout = new QHBoxLayout();
  setLayout(layout);

  prev_labelled_button = new QPushButton("Prev labelled");
  layout->addWidget(prev_labelled_button);
  prev_unlabelled_button = new QPushButton("Prev unlabelled");
  layout->addWidget(prev_unlabelled_button);
  prev_button = new QPushButton("Prev");
  layout->addWidget(prev_button);

  current_doc_label = new QLabel();
  layout->addWidget(current_doc_label);
  current_doc_label->setAlignment(Qt::AlignHCenter);

  next_button = new QPushButton("Next");
  layout->addWidget(next_button);
  next_unlabelled_button = new QPushButton("Next unlabelled");
  layout->addWidget(next_unlabelled_button);
  next_labelled_button = new QPushButton("Next labelled");
  layout->addWidget(next_labelled_button);

  QObject::connect(next_button, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_next);
  QObject::connect(prev_button, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_prev);
  QObject::connect(next_labelled_button, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_next_labelled);
  QObject::connect(prev_labelled_button, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_prev_labelled);
  QObject::connect(next_unlabelled_button, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_next_unlabelled);
  QObject::connect(prev_unlabelled_button, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_prev_unlabelled);
}

void AnnotationsNavButtons::setModel(AnnotationsModel* new_model) {
  annotations_model = new_model;
  QObject::connect(annotations_model, &AnnotationsModel::document_changed, this,
                   &AnnotationsNavButtons::update_button_states);
  update_button_states();
}

void AnnotationsNavButtons::update_button_states() {
  if (annotations_model == nullptr) {
    return;
  }

  auto doc_position = annotations_model->current_doc_position();
  auto total_docs = annotations_model->total_n_docs();
  auto new_msg = QString("%0 / %1").arg(doc_position + 1).arg(total_docs);
  if (total_docs == 0) {
    new_msg = QString("0 / 0");
  }
  current_doc_label->setText(new_msg);
  next_button->setEnabled(annotations_model->has_next());
  prev_button->setEnabled(annotations_model->has_prev());
  next_labelled_button->setEnabled(annotations_model->has_next_labelled());
  prev_labelled_button->setEnabled(annotations_model->has_prev_labelled());
  next_unlabelled_button->setEnabled(annotations_model->has_next_unlabelled());
  prev_unlabelled_button->setEnabled(annotations_model->has_prev_unlabelled());
}

} // namespace labelbuddy
