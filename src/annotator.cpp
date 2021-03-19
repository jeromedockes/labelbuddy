#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
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
#include "utils.h"

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
  label_delegate_.reset(new LabelDelegate);
  labels_view->setItemDelegate(label_delegate_.get());

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
  auto selected = find_first_in_col_0(selected_indexes);
  if (selected == selected_indexes.constEnd()) {
    return -1;
  }
  int label_id = label_list_model->data(*selected, Roles::RowIdRole).toInt();
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
bool LabelChoices::is_label_choice_enabled() const {
  return labels_view->isEnabled();
}

bool operator<(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  if (lhs.start_char < rhs.start_char) {
    return true;
  }
  if (lhs.start_char > rhs.start_char) {
    return false;
  }
  return lhs.id < rhs.id;
}

bool operator>(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  return rhs < lhs;
}

bool operator==(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  return (lhs.id == rhs.id) && (lhs.start_char == rhs.start_char);
}

bool operator!=(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  return !(lhs == rhs);
}

bool operator<=(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  return (lhs == rhs) || (lhs < rhs);
}

bool operator>=(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  return rhs <= lhs;
}

Annotator::Annotator(QWidget* parent) : QSplitter(parent) {
  label_choices = new LabelChoices();
  addWidget(label_choices);
  scale_margin(*label_choices, Side::Right);
  auto text_frame = new QFrame();
  addWidget(text_frame);
  auto text_layout = new QVBoxLayout();
  text_frame->setLayout(text_layout);
  text_layout->setContentsMargins(0, 0, 0, 0);
  nav_buttons = new AnnotationsNavButtons();
  text_layout->addWidget(nav_buttons);
  title_label = new QLabel();
  text_layout->addWidget(title_label);
  title_label->setAlignment(Qt::AlignHCenter);
  title_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  title_label->setOpenExternalLinks(true);
  title_label->setWordWrap(true);
  text = new SearchableText();
  text_layout->addWidget(text);
  scale_margin(*text, Side::Left);
  text->get_text_edit()->installEventFilter(this);
  text->get_text_edit()->viewport()->installEventFilter(this);
  default_format = text->get_text_edit()->textCursor().charFormat();
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
  QObject::connect(this, &Annotator::active_annotation_changed, this,
                   &Annotator::current_status_display_changed);
  QObject::connect(text->get_text_edit(), &QPlainTextEdit::selectionChanged, this,
                   &Annotator::update_label_choices_button_states);
  QObject::connect(text->get_text_edit(), &QPlainTextEdit::cursorPositionChanged,
                   this, &Annotator::activate_cluster_at_cursor_pos);

  setFocusProxy(text);
}

void Annotator::store_state() {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("Annotator/state", saveState());
}

StatusBarInfo Annotator::current_status_info() const {
  if (!annotations_model->is_positioned_on_valid_doc()) {
    return {"", "", ""};
  }
  StatusBarInfo status_info{};
  if (active_annotation != -1) {
    bool is_first = active_annotation == sorted_annotations_.cbegin()->id;
    bool is_first_in_group{};
    for (const auto& cluster : clusters_) {
      if (active_annotation == cluster.first_annotation.id) {
        is_first_in_group = true;
        break;
      }
    }
    status_info.annotation_info =
        QString("%0%1 %2, %3")
            .arg(is_first_in_group ? "^" : "")
            .arg(is_first ? "^" : "")
            .arg(annotations[active_annotation].start_char)
            .arg(annotations[active_annotation].end_char);
    status_info.annotation_label =
        labels[annotations[active_annotation].label_id].name;
  }
  status_info.doc_info = QString("%0 annotation%1 in current doc")
                             .arg(annotations.size())
                             .arg(annotations.size() != 1 ? "s" : "");
  return status_info;
}

void Annotator::set_font(const QFont& new_font) {
  text->get_text_edit()->setFont(new_font);
}

void Annotator::set_use_bold_font(bool bold_font) {
  use_bold_font = bold_font;
  paint_annotations();
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

void Annotator::clear_annotations() {
  deactivate_active_annotation();
  text->get_text_edit()->setExtraSelections({});
  annotations.clear();
  sorted_annotations_.clear();
  clusters_.clear();
}

void Annotator::reset_document() {
  clear_annotations();
  text->fill(annotations_model->get_content());
  title_label->setText(annotations_model->get_title());
  update_annotations();
  emit current_status_display_changed();
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

void Annotator::add_annotation_to_clusters(const AnnotationCursor& annotation,
                                           std::list<Cluster>& clusters) {
  Cluster new_cluster{{annotation.start_char, annotation.id},
                      {annotation.start_char, annotation.id},
                      annotation.start_char,
                      annotation.end_char};
  QList<std::list<Cluster>::const_iterator> clusters_to_remove{};
  for (auto c_it = clusters.cbegin(); c_it != clusters.cend(); ++c_it) {
    if (annotation.start_char < c_it->end_char &&
        c_it->start_char < annotation.end_char) {
      clusters_to_remove << c_it;
      new_cluster.first_annotation =
          std::min(new_cluster.first_annotation, c_it->first_annotation);
      new_cluster.last_annotation =
          std::max(new_cluster.last_annotation, c_it->last_annotation);
      new_cluster.start_char =
          std::min(new_cluster.start_char, c_it->start_char);
      new_cluster.end_char = std::max(new_cluster.end_char, c_it->end_char);
    }
  }
  for (auto c_it_to_remove : clusters_to_remove) {
    clusters.erase(c_it_to_remove);
  }
  clusters.push_back(new_cluster);
}

void Annotator::remove_annotation_from_clusters(
    const AnnotationCursor& annotation, std::list<Cluster>& clusters) {
  auto annotation_cluster = clusters.cbegin();
  while (annotation_cluster != clusters.cend()) {
    if (annotation_cluster->start_char <= annotation.start_char &&
        annotation_cluster->end_char >= annotation.end_char) {
      break;
    }
    ++annotation_cluster;
  }
  if (annotation_cluster == clusters.cend()) {
    return;
  }
  std::list<Cluster> new_clusters{};
  auto anno = sorted_annotations_.find(annotation_cluster->first_annotation);
  while (*anno != annotation_cluster->last_annotation) {
    if (anno->id != annotation.id) {
      add_annotation_to_clusters(annotations[anno->id], new_clusters);
    }
    ++anno;
  }
  if (anno->id != annotation.id) {
    add_annotation_to_clusters(annotations[anno->id], new_clusters);
  }
  clusters.erase(annotation_cluster);
  clusters.splice(clusters.cend(), new_clusters);
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

std::list<Cluster>::const_iterator Annotator::cluster_at_pos(int pos) const {
  for (auto c_it = clusters_.cbegin(); c_it != clusters_.cend(); ++c_it) {
    if (c_it->start_char <= pos && c_it->end_char > pos) {
      return c_it;
    }
  }
  return clusters_.cend();
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
  // we only set a charformat if necessary, to avoid re-wrapping long lines
  // unnecessarily when bold is not used
  if (active_anno_format_is_set_) {
    annotations[active_annotation].cursor.setCharFormat(default_format);
    active_anno_format_is_set_ = false;
  }
  active_annotation = -1;
}

void Annotator::activate_cluster_at_cursor_pos() {
  need_update_active_anno_ = false;
  auto cursor = text->get_text_edit()->textCursor();
  // only activate if click, not drag
  if (cursor.anchor() != cursor.position()) {
    if (active_annotation != -1) {
      deactivate_active_annotation();
      emit active_annotation_changed();
    }
    return;
  }
  int annotation_id{-1};
  auto cluster = cluster_at_pos(cursor.position());
  if (cluster != clusters_.cend()) {
    if (active_annotation == -1) {
      annotation_id = cluster->first_annotation.id;
    } else {
      auto active = annotations[active_annotation];
      auto active_index = AnnotationIndex{active.start_char, active.id};
      if (active_index < cluster->first_annotation ||
          active_index > cluster->last_annotation) {
        annotation_id = cluster->first_annotation.id;
      } else {
        auto it = sorted_annotations_.find(active_index);
        if (it->id == cluster->last_annotation.id) {
          annotation_id = cluster->first_annotation.id;
        } else {
          ++it;
          annotation_id = it->id;
        }
      }
    }
  }
  if (active_annotation == annotation_id) {
    return;
  }
  deactivate_active_annotation();
  active_annotation = annotation_id;
  emit active_annotation_changed();
}

void Annotator::delete_annotation(int annotation_id) {
  if (active_annotation == annotation_id) {
    deactivate_active_annotation();
  }
  auto anno = annotations[annotation_id];
  remove_annotation_from_clusters(anno, clusters_);
  annotations.remove(annotation_id);
  sorted_annotations_.erase({anno.start_char, anno.id});
  annotations_model->delete_annotations({annotation_id});
  emit active_annotation_changed();
}

void Annotator::delete_active_annotation() {
  delete_annotation(active_annotation);
  text->setFocus();
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
  int prev_active = active_annotation;
  if (add_annotation(label_id, start, end)) {
    delete_annotation(prev_active);
  }
}

bool Annotator::add_annotation() {
  int label_id = label_choices->selected_label_id();
  if (label_id == -1) {
    return false;
  }
  QList<int> selection_boundaries = text->current_selection();
  auto start = selection_boundaries[0];
  auto end = selection_boundaries[1];
  if (start == end) {
    return false;
  }
  return add_annotation(label_id, start, end);
}

bool Annotator::add_annotation(int label_id, int start_char, int end_char) {
  auto annotation_id =
      annotations_model->add_annotation(label_id, start_char, end_char);
  if (annotation_id == -1) {
    auto c = text->get_text_edit()->textCursor();
    c.clearSelection();
    text->get_text_edit()->setTextCursor(c);
    active_annotation = -1;
    emit active_annotation_changed();
    return false;
  }
  auto annotation_cursor = text->get_text_edit()->textCursor();
  annotation_cursor.setPosition(start_char);
  annotation_cursor.setPosition(end_char, QTextCursor::KeepAnchor);
  annotations[annotation_id] = AnnotationCursor{
      annotation_id, label_id, start_char, end_char, annotation_cursor};
  add_annotation_to_clusters(annotations[annotation_id], clusters_);
  sorted_annotations_.insert({start_char, annotation_id});
  auto new_cursor = text->get_text_edit()->textCursor();
  new_cursor.clearSelection();
  text->get_text_edit()->setTextCursor(new_cursor);
  deactivate_active_annotation();
  active_annotation = annotation_id;
  emit active_annotation_changed();
  return true;
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
  clear_annotations();
  auto annotation_positions = annotations_model->get_annotations_info();
  for (auto i = annotation_positions.constBegin();
       i != annotation_positions.constEnd(); ++i) {
    auto start = i.value().start_char;
    auto end = i.value().end_char;
    auto cursor = text->get_text_edit()->textCursor();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    annotations[i.value().id] =
        AnnotationCursor{i.value().id, i.value().label_id, start, end, cursor};
    sorted_annotations_.insert({start, i.value().id});
    add_annotation_to_clusters(annotations[i.value().id], clusters_);
  }
  if (annotations.contains(prev_active)) {
    active_annotation = prev_active;
  }
}

int Annotator::find_next_annotation(AnnotationIndex pos, bool forward) const {
  if (annotations.size() == 0) {
    return -1;
  }
  if (forward) {
    auto i = sorted_annotations_.lower_bound(pos);
    if (i != sorted_annotations_.cend()) {
      return i->id;
    }
    return sorted_annotations_.cbegin()->id;
  }
  auto i = sorted_annotations_.crbegin();
  while ((i != sorted_annotations_.crend()) && (*i > pos)) {
    ++i;
  }
  if (i != sorted_annotations_.crend()) {
    return i->id;
  }
  return (sorted_annotations_.crbegin())->id;
}

void Annotator::select_next_annotation(bool forward) {
  AnnotationIndex pos{};
  if (active_annotation != -1) {
    int offset = forward ? 1 : -1;
    pos = {annotations[active_annotation].start_char,
           active_annotation + offset};
  } else {
    pos = {text->get_text_edit()->textCursor().position(), 0};
  }
  auto next_anno = find_next_annotation(pos, forward);
  if (next_anno == -1) {
    return;
  }
  auto cursor = text->get_text_edit()->textCursor();
  cursor.setPosition(annotations[next_anno].start_char);
  text->get_text_edit()->setTextCursor(cursor);
  text->get_text_edit()->ensureCursorVisible();
  deactivate_active_annotation();
  active_annotation = next_anno;
  emit active_annotation_changed();
}

QTextEdit::ExtraSelection
Annotator::make_painted_region(int start_char, int end_char,
                               const QString& color, const QString& text_color,
                               bool underline) {
  auto cursor = text->get_text_edit()->textCursor();
  cursor.setPosition(start_char);
  cursor.setPosition(end_char, QTextCursor::KeepAnchor);

  QTextCharFormat format(default_format);
  format.setBackground(QColor(color));
  format.setForeground(QColor(text_color));
  format.setFontUnderline(underline);

  return QTextEdit::ExtraSelection{cursor, format};
}

void Annotator::paint_annotations() {
  const QString dark_gray{"#606060"};
  QList<QTextEdit::ExtraSelection> new_selections{};
  int active_start{-1};
  int active_end{-1};
  if (active_annotation != -1) {
    active_start = annotations[active_annotation].start_char;
    active_end = annotations[active_annotation].end_char;
  }
  for (const auto& cluster : clusters_) {
    auto cluster_start = cluster.start_char;
    auto cluster_end = cluster.end_char;
    if (cluster.last_annotation.id != cluster.first_annotation.id) {
      if ((cluster_start < active_start) && (active_start < cluster_end)) {
        new_selections << make_painted_region(cluster_start, active_start,
                                              dark_gray, "white");
      }
      if ((cluster_start < active_end) && (cluster_end > active_end)) {
        new_selections << make_painted_region(active_end, cluster_end,
                                              dark_gray, "white");
      }
      if ((active_end <= cluster_start) || (active_start >= cluster_end)) {
        new_selections << make_painted_region(cluster_start, cluster_end,
                                              dark_gray, "white");
      }
    } else if (cluster.first_annotation.id != active_annotation) {
      new_selections << make_painted_region(
          cluster_start, cluster_end,
          labels.value(annotations.value(cluster.first_annotation.id).label_id)
              .color);
    }
  }
  if (active_annotation != -1) {
    auto anno = annotations[active_annotation];
    if (use_bold_font) {
      QTextCharFormat fmt(default_format);
      fmt.setFontWeight(QFont::Bold);
      anno.cursor.setCharFormat(fmt);
      active_anno_format_is_set_ = true;
    } else if (active_anno_format_is_set_) {
      annotations[active_annotation].cursor.setCharFormat(default_format);
      active_anno_format_is_set_ = false;
    }
    new_selections << make_painted_region(anno.start_char, anno.end_char,
                                          labels[anno.label_id].color, "black",
                                          true);
  }
  text->get_text_edit()->setExtraSelections(new_selections);
}

bool Annotator::eventFilter(QObject* object, QEvent* event) {
  if (object == text->get_text_edit()) {
    if (event->type() == QEvent::KeyPress) {
      auto key_event = static_cast<QKeyEvent*>(event);
      if (key_event->key() == Qt::Key_Space) {
        keyPressEvent(key_event);
        return true;
      }
    }
  }
  if (event->type() == QEvent::MouseButtonPress) {
    mousePressEvent(static_cast<QMouseEvent*>(event));
    return false;
  }
  if (event->type() == QEvent::MouseButtonRelease) {
    mouseReleaseEvent(static_cast<QMouseEvent*>(event));
    return false;
  }
  return QWidget::eventFilter(object, event);
}

void Annotator::mousePressEvent(QMouseEvent* event) {
  (void)event;
  need_update_active_anno_ = true;
}

void Annotator::mouseReleaseEvent(QMouseEvent* event) {
  (void)event;
  if (need_update_active_anno_) {
    activate_cluster_at_cursor_pos();
  }
}

void Annotator::keyPressEvent(QKeyEvent* event) {
  if (event->text() == ">") {
    annotations_model->visit_next();
    return;
  }
  if (event->text() == "<") {
    annotations_model->visit_prev();
    return;
  }
  if (event->key() == Qt::Key_Escape) {
    deactivate_active_annotation();
    emit active_annotation_changed();
  }
  if (event->key() == Qt::Key_Space) {
    bool backward(event->modifiers() & Qt::ShiftModifier);
    select_next_annotation(!backward);
    return;
  }
  if (!label_choices->is_label_choice_enabled()) {
    return;
  }
  auto id = annotations_model->shortcut_to_id(event->text());
  if (id != -1) {
    label_choices->set_selected_label_id(id);
    return;
  }
  if (event->key() == Qt::Key_Backspace) {
    delete_active_annotation();
    return;
  }
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
