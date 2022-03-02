#include <cassert>

#include <QColor>
#include <QElapsedTimer>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
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
  instruction_label_ = new QLabel("Set label for selected text:");
  layout->addWidget(instruction_label_);
  instruction_label_->setWordWrap(true);
  this->setStyleSheet("QListView::item {background: transparent;}");
  labels_view_ = new NoDeselectAllView();
  layout->addWidget(labels_view_);
  // labels_view->setSpacing(3);
  labels_view_->setFocusPolicy(Qt::NoFocus);
  label_delegate_.reset(new LabelDelegate);
  labels_view_->setItemDelegate(label_delegate_.get());

  extra_data_label_ = new QLabel("&Extra annotation data:");
  layout->addWidget(extra_data_label_);
  extra_data_label_->setWordWrap(true);
  extra_data_edit_ = new QLineEdit();
  layout->addWidget(extra_data_edit_);
  extra_data_label_->setBuddy(extra_data_edit_);

  delete_button_ = new QPushButton("Delete annotation");
  layout->addWidget(delete_button_);

  QObject::connect(delete_button_, &QPushButton::clicked, this,
                   &LabelChoices::on_delete_button_click);
  QObject::connect(extra_data_edit_, &QLineEdit::textEdited, this,
                   &LabelChoices::extra_data_edited);
  QObject::connect(extra_data_edit_, &QLineEdit::returnPressed, this,
                   &LabelChoices::extra_data_edit_finished);
}

void LabelChoices::setModel(LabelListModel* new_model) {
  assert(new_model != nullptr);
  label_list_model_ = new_model;
  labels_view_->setModel(new_model);
  QObject::connect(labels_view_->selectionModel(),
                   &QItemSelectionModel::selectionChanged, this,
                   &LabelChoices::on_selection_change);
}

void LabelChoices::on_selection_change() { emit selectionChanged(); }
void LabelChoices::on_delete_button_click() { emit delete_button_clicked(); }

QModelIndexList LabelChoices::selectedIndexes() const {
  return labels_view_->selectionModel()->selectedIndexes();
}

int LabelChoices::selected_label_id() const {
  if (label_list_model_ == nullptr) {
    assert(false);
    return -1;
  }
  QModelIndexList selected_indexes =
      labels_view_->selectionModel()->selectedIndexes();
  auto selected = find_first_in_col_0(selected_indexes);
  if (selected == selected_indexes.constEnd()) {
    return -1;
  }
  int label_id = label_list_model_->data(*selected, Roles::RowIdRole).toInt();
  return label_id;
}

void LabelChoices::set_selected_label_id(int label_id) {
  if (label_list_model_ == nullptr) {
    return;
  }
  if (label_id == -1) {
    labels_view_->clearSelection();
    return;
  }
  auto model_index = label_list_model_->label_id_to_model_index(label_id);
  if (!model_index.isValid()) {
    return;
  }
  labels_view_->setCurrentIndex(model_index);
}

void LabelChoices::set_extra_data(const QString& new_data) {
  extra_data_edit_->setText(new_data);
  extra_data_edit_->setCursorPosition(0);
}

void LabelChoices::enable_delete_and_edit() {
  delete_button_->setEnabled(true);
  extra_data_edit_->setEnabled(true);
  extra_data_label_->setEnabled(true);
}

void LabelChoices::disable_delete_and_edit() {
  delete_button_->setDisabled(true);
  extra_data_edit_->setDisabled(true);
  extra_data_label_->setDisabled(true);
}

void LabelChoices::enable_label_choice() { labels_view_->setEnabled(true); }
void LabelChoices::disable_label_choice() { labels_view_->setDisabled(true); }
bool LabelChoices::is_label_choice_enabled() const {
  return labels_view_->isEnabled();
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
  label_choices_ = new LabelChoices();
  addWidget(label_choices_);
  scale_margin(*label_choices_, Side::Right);
  auto text_frame = new QFrame();
  addWidget(text_frame);
  auto text_layout = new QVBoxLayout();
  text_frame->setLayout(text_layout);
  text_layout->setContentsMargins(0, 0, 0, 0);
  nav_buttons_ = new AnnotationsNavButtons();
  text_layout->addWidget(nav_buttons_);
  title_label_ = new QLabel();
  text_layout->addWidget(title_label_);
  title_label_->setAlignment(Qt::AlignHCenter);
  title_label_->setTextInteractionFlags(Qt::TextBrowserInteraction);
  title_label_->setOpenExternalLinks(true);
  title_label_->setWordWrap(true);
  text_ = new SearchableText();
  text_layout->addWidget(text_);
  scale_margin(*text_, Side::Left);
  text_->get_text_edit()->installEventFilter(this);
  text_->get_text_edit()->viewport()->installEventFilter(this);
  default_format_ = text_->get_text_edit()->textCursor().charFormat();
  text_->fill("");
  text_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  QSettings settings("labelbuddy", "labelbuddy");
  if (settings.contains("Annotator/state")) {
    restoreState(settings.value("Annotator/state").toByteArray());
  }

  QObject::connect(label_choices_, &LabelChoices::selectionChanged, this,
                   &Annotator::set_label_for_selected_region);
  QObject::connect(label_choices_, &LabelChoices::delete_button_clicked, this,
                   &Annotator::delete_active_annotation);
  QObject::connect(label_choices_, &LabelChoices::extra_data_edited, this,
                   &Annotator::update_extra_data_for_active_annotation);
  QObject::connect(label_choices_, &LabelChoices::extra_data_edit_finished, this,
                   &Annotator::set_default_focus);
  QObject::connect(this, &Annotator::active_annotation_changed, this,
                   &Annotator::paint_annotations);
  QObject::connect(this, &Annotator::active_annotation_changed, this,
                   &Annotator::update_label_choices_button_states);
  QObject::connect(this, &Annotator::active_annotation_changed, this,
                   &Annotator::current_status_display_changed);
  QObject::connect(text_->get_text_edit(), &QPlainTextEdit::selectionChanged,
                   this, &Annotator::update_label_choices_button_states);
  QObject::connect(text_->get_text_edit(),
                   &QPlainTextEdit::cursorPositionChanged, this,
                   &Annotator::activate_cluster_at_cursor_pos);

  setFocusProxy(text_);
}

void Annotator::store_state() {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("Annotator/state", saveState());
}

StatusBarInfo Annotator::current_status_info() const {
  if (!annotations_model_->is_positioned_on_valid_doc()) {
    return {"", "", ""};
  }
  StatusBarInfo status_info{};
  if (active_annotation_ != -1) {
    bool is_first = active_annotation_ == sorted_annotations_.cbegin()->id;
    bool is_first_in_group{};
    for (const auto& cluster : clusters_) {
      if (active_annotation_ == cluster.first_annotation.id) {
        is_first_in_group = true;
        break;
      }
    }
    status_info.annotation_info =
        QString("%0%1 %2, %3")
            .arg(is_first_in_group ? "^" : "")
            .arg(is_first ? "^" : "")
            .arg(annotations_model_->utf16_idx_to_code_point_idx(
                annotations_[active_annotation_].start_char))
            .arg(annotations_model_->utf16_idx_to_code_point_idx(
                annotations_[active_annotation_].end_char));
    status_info.annotation_label =
        labels_[annotations_[active_annotation_].label_id].name;
  }
  status_info.doc_info = QString("%0 annotation%1 in current doc")
                             .arg(annotations_.size())
                             .arg(annotations_.size() != 1 ? "s" : "");
  return status_info;
}

void Annotator::set_font(const QFont& new_font) {
  text_->get_text_edit()->setFont(new_font);
}

void Annotator::set_use_bold_font(bool bold_font) {
  use_bold_font_ = bold_font;
  paint_annotations();
}

void Annotator::set_annotations_model(AnnotationsModel* new_model) {
  assert(new_model != nullptr);
  annotations_model_ = new_model;
  nav_buttons_->setModel(new_model);

  QObject::connect(annotations_model_, &AnnotationsModel::document_changed, this,
                   &Annotator::reset_document);
  QObject::connect(nav_buttons_, &AnnotationsNavButtons::visit_next,
                   annotations_model_, &AnnotationsModel::visit_next);
  QObject::connect(nav_buttons_, &AnnotationsNavButtons::visit_prev,
                   annotations_model_, &AnnotationsModel::visit_prev);
  QObject::connect(nav_buttons_, &AnnotationsNavButtons::visit_next_labelled,
                   annotations_model_, &AnnotationsModel::visit_next_labelled);
  QObject::connect(nav_buttons_, &AnnotationsNavButtons::visit_prev_labelled,
                   annotations_model_, &AnnotationsModel::visit_prev_labelled);
  QObject::connect(nav_buttons_, &AnnotationsNavButtons::visit_next_unlabelled,
                   annotations_model_, &AnnotationsModel::visit_next_unlabelled);
  QObject::connect(nav_buttons_, &AnnotationsNavButtons::visit_prev_unlabelled,
                   annotations_model_, &AnnotationsModel::visit_prev_unlabelled);

  reset_document();
}

void Annotator::clear_annotations() {
  deactivate_active_annotation();
  text_->get_text_edit()->setExtraSelections({});
  annotations_.clear();
  sorted_annotations_.clear();
  clusters_.clear();
}

void Annotator::reset_document() {
  clear_annotations();
  text_->fill(annotations_model_->get_content());
  title_label_->setText(annotations_model_->get_title());
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
  assert(new_model != nullptr);
  label_choices_->setModel(new_model);
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
  // find the annotation's cluster
  auto annotation_cluster = clusters.cbegin();
  while (annotation_cluster != clusters.cend()) {
    if (annotation_cluster->start_char <= annotation.start_char &&
        annotation_cluster->end_char >= annotation.end_char) {
      break;
    }
    ++annotation_cluster;
  }
  if (annotation_cluster == clusters.cend()) {
    assert(false);
    return;
  }
  // remove the annotation's cluster and re-insert the other annotations it
  // contains.
  std::list<Cluster> new_clusters{};
  auto anno = sorted_annotations_.find(annotation_cluster->first_annotation);
  assert(anno != sorted_annotations_.end());
  while (*anno != annotation_cluster->last_annotation) {
    if (anno->id != annotation.id) {
      add_annotation_to_clusters(annotations_[anno->id], new_clusters);
    }
    ++anno;
    assert(anno != sorted_annotations_.end());
  }
  if (anno->id != annotation.id) {
    add_annotation_to_clusters(annotations_[anno->id], new_clusters);
  }
  clusters.erase(annotation_cluster);
  clusters.splice(clusters.cend(), new_clusters);
}

void Annotator::update_label_choices_button_states() {
  label_choices_->set_selected_label_id(active_annotation_label());
  label_choices_->set_extra_data(active_annotation_extra_data());
  if (active_annotation_ != -1) {
    label_choices_->enable_delete_and_edit();
    label_choices_->enable_label_choice();
    return;
  }
  label_choices_->disable_delete_and_edit();
  auto selection = text_->current_selection();
  if (selection[0] != selection[1]) {
    label_choices_->enable_label_choice();
  } else {
    label_choices_->disable_label_choice();
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

void Annotator::update_nav_buttons() { nav_buttons_->update_button_states(); }

void Annotator::reset_skip_updating_nav_buttons() {
  nav_buttons_->set_skip_updating(false);
}

int Annotator::active_annotation_label() const {
  if (active_annotation_ == -1) {
    return -1;
  }
  return annotations_[active_annotation_].label_id;
}

QString Annotator::active_annotation_extra_data() const {
  if (active_annotation_ == -1) {
    return "";
  }
  return annotations_[active_annotation_].extra_data;
}

void Annotator::deactivate_active_annotation() {
  if (!annotations_.contains(active_annotation_)) {
    return;
  }
  // we only set a charformat if necessary, to avoid re-wrapping long lines
  // unnecessarily when bold is not used
  if (active_anno_format_is_set_) {
    annotations_[active_annotation_].cursor.setCharFormat(default_format_);
    active_anno_format_is_set_ = false;
  }
  active_annotation_ = -1;
}

void Annotator::activate_cluster_at_cursor_pos() {
  need_update_active_anno_ = false;
  auto cursor = text_->get_text_edit()->textCursor();
  // only activate if click, not drag
  if (cursor.anchor() != cursor.position()) {
    if (active_annotation_ != -1) {
      deactivate_active_annotation();
      emit active_annotation_changed();
    }
    return;
  }
  int annotation_id{-1};
  auto cluster = cluster_at_pos(cursor.position());
  if (cluster != clusters_.cend()) {
    if (active_annotation_ == -1) {
      annotation_id = cluster->first_annotation.id;
    } else {
      auto active = annotations_[active_annotation_];
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
  if (active_annotation_ == annotation_id) {
    return;
  }
  deactivate_active_annotation();
  active_annotation_ = annotation_id;
  emit active_annotation_changed();
}

void Annotator::delete_annotation(int annotation_id) {
  if (annotation_id == -1){
    return;
  }
  if (active_annotation_ == annotation_id) {
    deactivate_active_annotation();
  }
  auto deleted = annotations_model_->delete_annotation(annotation_id);
  if (!deleted) {
    assert(false);
    emit active_annotation_changed();
    return;
  }
  auto anno = annotations_[annotation_id];
  remove_annotation_from_clusters(anno, clusters_);
  annotations_.remove(annotation_id);
  sorted_annotations_.erase({anno.start_char, anno.id});
  emit active_annotation_changed();
}

void Annotator::delete_active_annotation() {
  delete_annotation(active_annotation_);
  set_default_focus();
}

void Annotator::set_default_focus() { text_->setFocus(); }

void Annotator::update_extra_data_for_active_annotation(
    const QString& new_data) {
  if (active_annotation_ == -1) {
    assert(false);
    return;
  }
  if (annotations_model_->update_annotation_extra_data(active_annotation_,
                                                      new_data)) {
    annotations_[active_annotation_].extra_data = new_data;
  } else {
    assert(false);
  }
}

void Annotator::set_label_for_selected_region() {
  if (active_annotation_ == -1) {
    add_annotation();
    return;
  }
  int label_id = label_choices_->selected_label_id();
  auto prev_label = annotations_[active_annotation_].label_id;
  if (label_id == prev_label) {
    return;
  }
  auto start = annotations_[active_annotation_].start_char;
  auto end = annotations_[active_annotation_].end_char;
  int prev_active = active_annotation_;
  if (add_annotation(label_id, start, end)) {
    delete_annotation(prev_active);
  }
}

bool Annotator::add_annotation() {
  int label_id = label_choices_->selected_label_id();
  if (label_id == -1) {
    return false;
  }
  QList<int> selection_boundaries = text_->current_selection();
  auto start = selection_boundaries[0];
  auto end = selection_boundaries[1];
  if (start == end) {
    assert(false);
    return false;
  }
  return add_annotation(label_id, start, end);
}

bool Annotator::add_annotation(int label_id, int start_char, int end_char) {
  auto annotation_id =
      annotations_model_->add_annotation(label_id, start_char, end_char);
  if (annotation_id == -1) {
    auto c = text_->get_text_edit()->textCursor();
    c.clearSelection();
    text_->get_text_edit()->setTextCursor(c);
    active_annotation_ = -1;
    emit active_annotation_changed();
    return false;
  }
  auto annotation_cursor = text_->get_text_edit()->textCursor();
  annotation_cursor.setPosition(start_char);
  annotation_cursor.setPosition(end_char, QTextCursor::KeepAnchor);
  annotations_[annotation_id] =
      AnnotationCursor{annotation_id, label_id,  start_char,
                       end_char,      QString(), annotation_cursor};
  add_annotation_to_clusters(annotations_[annotation_id], clusters_);
  sorted_annotations_.insert({start_char, annotation_id});
  auto new_cursor = text_->get_text_edit()->textCursor();
  new_cursor.clearSelection();
  text_->get_text_edit()->setTextCursor(new_cursor);
  deactivate_active_annotation();
  active_annotation_ = annotation_id;
  emit active_annotation_changed();
  return true;
}

void Annotator::fetch_labels_info() {
  if (annotations_model_ == nullptr) {
    assert(false);
    return;
  }
  labels_ = annotations_model_->get_labels_info();
}

void Annotator::fetch_annotations_info() {
  if (annotations_model_ == nullptr) {
    assert(false);
    return;
  }
  int prev_active{active_annotation_};
  clear_annotations();
  auto annotation_positions = annotations_model_->get_annotations_info();
  for (auto i = annotation_positions.constBegin();
       i != annotation_positions.constEnd(); ++i) {
    auto start = i.value().start_char;
    auto end = i.value().end_char;
    auto cursor = text_->get_text_edit()->textCursor();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    annotations_[i.value().id] =
        AnnotationCursor{i.value().id, i.value().label_id,   start,
                         end,          i.value().extra_data, cursor};
    sorted_annotations_.insert({start, i.value().id});
    add_annotation_to_clusters(annotations_[i.value().id], clusters_);
  }
  if (annotations_.contains(prev_active)) {
    active_annotation_ = prev_active;
  }
}

int Annotator::find_next_annotation(AnnotationIndex pos, bool forward) const {
  if (annotations_.size() == 0) {
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
  if (active_annotation_ != -1) {
    int offset = forward ? 1 : -1;
    pos = {annotations_[active_annotation_].start_char,
           active_annotation_ + offset};
  } else {
    pos = {text_->get_text_edit()->textCursor().position(), 0};
  }
  auto next_anno = find_next_annotation(pos, forward);
  if (next_anno == -1) {
    return;
  }
  auto cursor = text_->get_text_edit()->textCursor();
  cursor.setPosition(annotations_[next_anno].start_char);
  text_->get_text_edit()->setTextCursor(cursor);
  text_->get_text_edit()->ensureCursorVisible();
  deactivate_active_annotation();
  active_annotation_ = next_anno;
  emit active_annotation_changed();
}

QTextEdit::ExtraSelection
Annotator::make_painted_region(int start_char, int end_char,
                               const QString& color, const QString& text_color,
                               bool underline) {
  auto cursor = text_->get_text_edit()->textCursor();
  cursor.setPosition(start_char);
  cursor.setPosition(end_char, QTextCursor::KeepAnchor);

  QTextCharFormat format(default_format_);
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
  if (active_annotation_ != -1) {
    active_start = annotations_[active_annotation_].start_char;
    active_end = annotations_[active_annotation_].end_char;
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
    } else if (cluster.first_annotation.id != active_annotation_) {
      new_selections << make_painted_region(
          cluster_start, cluster_end,
          labels_.value(annotations_.value(cluster.first_annotation.id).label_id)
              .color);
    }
  }
  if (active_annotation_ != -1) {
    auto anno = annotations_[active_annotation_];
    if (use_bold_font_) {
      QTextCharFormat fmt(default_format_);
      fmt.setFontWeight(QFont::Bold);
      anno.cursor.setCharFormat(fmt);
      active_anno_format_is_set_ = true;
    } else if (active_anno_format_is_set_) {
      annotations_[active_annotation_].cursor.setCharFormat(default_format_);
      active_anno_format_is_set_ = false;
    }
    new_selections << make_painted_region(anno.start_char, anno.end_char,
                                          labels_[anno.label_id].color, "black",
                                          true);
  }
  text_->get_text_edit()->setExtraSelections(new_selections);
}

bool Annotator::eventFilter(QObject* object, QEvent* event) {
  if (object == text_->get_text_edit()) {
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
    annotations_model_->visit_next();
    return;
  }
  if (event->text() == "<") {
    annotations_model_->visit_prev();
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
  if (!label_choices_->is_label_choice_enabled()) {
    return;
  }
  auto id = annotations_model_->shortcut_to_id(event->text());
  if (id != -1) {
    label_choices_->set_selected_label_id(id);
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

  auto next_icon =
      QIcon::fromTheme("go-next", QIcon(":data/icons/go-next.png"));
  auto prev_icon =
      QIcon::fromTheme("go-previous", QIcon(":data/icons/go-previous.png"));
  prev_labelled_button_ = new QPushButton(prev_icon, "labelled");
  layout->addWidget(prev_labelled_button_);
  prev_unlabelled_button_ = new QPushButton(prev_icon, "unlabelled");
  layout->addWidget(prev_unlabelled_button_);
  prev_button_ = new QPushButton(prev_icon, "");
  layout->addWidget(prev_button_);

  current_doc_label_ = new QLabel();
  layout->addWidget(current_doc_label_);
  current_doc_label_->setAlignment(Qt::AlignCenter);

  next_button_ = new QPushButton(next_icon, "");
  layout->addWidget(next_button_);
  next_unlabelled_button_ = new QPushButton(next_icon, "unlabelled");
  layout->addWidget(next_unlabelled_button_);
  next_labelled_button_ = new QPushButton(next_icon, "labelled");
  layout->addWidget(next_labelled_button_);

  QObject::connect(next_button_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_next);
  QObject::connect(prev_button_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_prev);
  QObject::connect(next_labelled_button_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_next_labelled);
  QObject::connect(prev_labelled_button_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_prev_labelled);
  QObject::connect(next_unlabelled_button_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_next_unlabelled);
  QObject::connect(prev_unlabelled_button_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visit_prev_unlabelled);
}

void AnnotationsNavButtons::setModel(AnnotationsModel* new_model) {
  assert(new_model != nullptr);
  annotations_model_ = new_model;
  QObject::connect(annotations_model_, &AnnotationsModel::document_changed, this,
                   &AnnotationsNavButtons::update_button_states);
  update_button_states();
}

void AnnotationsNavButtons::update_button_states() {
  if (annotations_model_ == nullptr) {
    assert(false);
    return;
  }
  auto doc_position = annotations_model_->current_doc_position();
  auto total_docs = annotations_model_->total_n_docs();
  auto new_msg = QString("%0 / %1").arg(doc_position + 1).arg(total_docs);
  if (total_docs == 0) {
    new_msg = QString("0 / 0");
  }
  current_doc_label_->setText(new_msg);
  if (skip_updating_buttons_) {
    return;
  }
  QElapsedTimer timer{};
  timer.start();
  next_button_->setEnabled(annotations_model_->has_next());
  prev_button_->setEnabled(annotations_model_->has_prev());
  next_labelled_button_->setEnabled(annotations_model_->has_next_labelled());
  if (timer.elapsed() > 500)
    return set_skip_updating(true);
  prev_labelled_button_->setEnabled(annotations_model_->has_prev_labelled());
  if (timer.elapsed() > 500)
    return set_skip_updating(true);
  next_unlabelled_button_->setEnabled(annotations_model_->has_next_unlabelled());
  if (timer.elapsed() > 500)
    return set_skip_updating(true);
  prev_unlabelled_button_->setEnabled(annotations_model_->has_prev_unlabelled());
  if (timer.elapsed() > 500)
    return set_skip_updating(true);
}

void AnnotationsNavButtons::set_skip_updating(bool skip) {
  if (skip) {
    skip_updating_buttons_ = true;
    next_button_->setEnabled(true);
    prev_button_->setEnabled(true);
    next_labelled_button_->setEnabled(true);
    prev_labelled_button_->setEnabled(true);
    next_unlabelled_button_->setEnabled(true);
    prev_unlabelled_button_->setEnabled(true);
    return;
  }
  if (skip_updating_buttons_) {
    skip_updating_buttons_ = false;
  }
}

} // namespace labelbuddy
