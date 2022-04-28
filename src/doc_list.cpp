#include <cassert>

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "doc_list.h"
#include "user_roles.h"
#include "utils.h"

namespace labelbuddy {
DocListButtons::DocListButtons(QWidget* parent) : QFrame(parent) {
  auto main_layout = new QVBoxLayout();
  setLayout(main_layout);
  auto buttons_layout = new QHBoxLayout();
  main_layout->addLayout(buttons_layout);
  auto filters_layout = new QHBoxLayout();
  main_layout->addLayout(filters_layout);
  auto nav_layout = new QHBoxLayout();
  main_layout->addLayout(nav_layout);

  select_all_button_ = new QPushButton("Select all");
  buttons_layout->addWidget(select_all_button_);
  delete_button_ = new QPushButton("Delete");
  buttons_layout->addWidget(delete_button_);
  delete_all_button_ = new QPushButton("Delete all docs");
  buttons_layout->addWidget(delete_all_button_);
  annotate_button_ = new QPushButton("Annotate selected doc");
  buttons_layout->addWidget(annotate_button_);

  filters_layout->addWidget(new QLabel("Filter by label: "));
  filter_choice_ = new QComboBox();
  filters_layout->addWidget(filter_choice_);
  filters_layout->addStretch();

  first_page_button_ = new QPushButton(
      QIcon::fromTheme("go-first", QIcon(":data/icons/go-first.png")), "");
  nav_layout->addWidget(first_page_button_);
  prev_page_button_ = new QPushButton(
      QIcon::fromTheme("go-previous", QIcon(":data/icons/go-previous.png")),
      "");
  nav_layout->addWidget(prev_page_button_);
  current_page_label_ = new QLabel();
  nav_layout->addWidget(current_page_label_);
  current_page_label_->setAlignment(Qt::AlignHCenter);
  next_page_button_ = new QPushButton(
      QIcon::fromTheme("go-next", QIcon(":data/icons/go-next.png")), "");
  nav_layout->addWidget(next_page_button_);
  last_page_button_ = new QPushButton(
      QIcon::fromTheme("go-last", QIcon(":data/icons/go-last.png")), "");
  nav_layout->addWidget(last_page_button_);

  add_connections();
}

void DocListButtons::add_connections() {
  QObject::connect(next_page_button_, &QPushButton::clicked, this,
                   &DocListButtons::go_to_next_page);

  QObject::connect(prev_page_button_, &QPushButton::clicked, this,
                   &DocListButtons::go_to_prev_page);

  QObject::connect(first_page_button_, &QPushButton::clicked, this,
                   &DocListButtons::go_to_first_page);

  QObject::connect(last_page_button_, &QPushButton::clicked, this,
                   &DocListButtons::go_to_last_page);

  QObject::connect(select_all_button_, &QPushButton::clicked, this,
                   &DocListButtons::select_all);

  QObject::connect(delete_button_, &QPushButton::clicked, this,
                   &DocListButtons::delete_selected_rows);

  QObject::connect(delete_all_button_, &QPushButton::clicked, this,
                   &DocListButtons::delete_all_docs);

  QObject::connect(annotate_button_, &QPushButton::clicked, this,
                   &DocListButtons::visit_doc);

  void (QComboBox::*combobox_activated)(int) = &QComboBox::activated;
  QObject::connect(filter_choice_, combobox_activated, this,
                   &DocListButtons::update_filter);
  // note above could be simplified: QOverload<int>::of(&QComboBox::activated)
  // but QOverload introduced in qt 5.7 and xenial comes with 5.5
}

void DocListButtons::fill_filter_choice() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  filter_choice_->clear();
  QVariant var{};
  var.setValue(
      QPair<int, int>(-1, static_cast<int>(DocListModel::DocFilter::all)));
  filter_choice_->addItem("All documents", var);
  var.setValue(
      QPair<int, int>(-1, static_cast<int>(DocListModel::DocFilter::labelled)));
  filter_choice_->addItem("Documents with any label", var);
  var.setValue(QPair<int, int>(
      -1, static_cast<int>(DocListModel::DocFilter::unlabelled)));
  filter_choice_->addItem("Documents without labels", var);
  auto label_names = model_->get_label_names();
  if (!label_names.empty()) {
    filter_choice_->insertSeparator(filter_choice_->count());
  }
  for (const auto& label_info : label_names) {
    var.setValue(QPair<int, int>(
        label_info.second,
        static_cast<int>(DocListModel::DocFilter::has_given_label)));
    filter_choice_->addItem(label_info.first, var);
  }
  if (!label_names.empty()) {
    filter_choice_->insertSeparator(filter_choice_->count());
  }
  for (const auto& label_info : label_names) {
    var.setValue(QPair<int, int>(
        label_info.second,
        static_cast<int>(DocListModel::DocFilter::not_has_given_label)));
    filter_choice_->addItem(QString("NOT  %0").arg(label_info.first), var);
  }
  int label_idx{-1};
  switch (current_filter_) {
  case DocListModel::DocFilter::all:
    filter_choice_->setCurrentIndex(0);
    break;
  case DocListModel::DocFilter::labelled:
    filter_choice_->setCurrentIndex(1);
    break;
  case DocListModel::DocFilter::unlabelled:
    filter_choice_->setCurrentIndex(2);
    break;
  case DocListModel::DocFilter::has_given_label:
  case DocListModel::DocFilter::not_has_given_label:
    var.setValue(
        QPair<int, int>(current_label_id_, static_cast<int>(current_filter_)));
    label_idx = filter_choice_->findData(var);
    if (label_idx != -1) {
      filter_choice_->setCurrentIndex(label_idx);
    } else {
      filter_choice_->setCurrentIndex(0);
    }
    break;
  default:
    assert(false);
    filter_choice_->setCurrentIndex(0);
    break;
  }
  update_filter();
}

void DocListButtons::after_database_change() {
  current_filter_ = DocListModel::DocFilter::all;
  current_label_id_ = -1;
  offset_ = 0;
}

void DocListButtons::go_to_next_page() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  int total = model_->total_n_docs(current_filter_, current_label_id_);
  if (offset_ + page_size_ >= total) {
    return;
  }
  offset_ += page_size_;
  emit doc_filter_changed(current_filter_, current_label_id_, page_size_,
                          offset_);
}

void DocListButtons::go_to_prev_page() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  if (offset_ == 0) {
    return;
  }
  offset_ = std::max(0, offset_ - page_size_);
  emit doc_filter_changed(current_filter_, current_label_id_, page_size_,
                          offset_);
}

void DocListButtons::go_to_last_page() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  int total = model_->total_n_docs(current_filter_, current_label_id_);
  offset_ = total - total % page_size_;
  emit doc_filter_changed(current_filter_, current_label_id_, page_size_,
                          offset_);
}

void DocListButtons::go_to_first_page() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  offset_ = 0;
  emit doc_filter_changed(current_filter_, current_label_id_, page_size_,
                          offset_);
}

void DocListButtons::update_after_data_change() {
  int total = model_->total_n_docs(current_filter_, current_label_id_);
  int prev_offset = offset_;
  offset_ = std::max(0, std::min(total - 1, offset_));
  offset_ = offset_ - offset_ % page_size_;
  if (prev_offset != offset_) {
    emit doc_filter_changed(current_filter_, current_label_id_, page_size_,
                            offset_);
  }
  update_button_states();
  assert(offset_ >= 0);
  assert(!(offset_ % page_size_));
  assert(total == 0 || offset_ < total);
}

void DocListButtons::update_button_states() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }

  int total = model_->total_n_docs(current_filter_, current_label_id_);
  int end = offset_ + model_->rowCount();
  assert(total == 0 || offset_ < total);
  assert(end <= total);

  if (total == 0) {
    current_page_label_->setText("0 / 0");
  } else if (offset_ == end - 1) {
    current_page_label_->setText(QString("%1 / %2").arg(end).arg(total));
  } else {
    current_page_label_->setText(
        QString("%1 - %2 / %3").arg(offset_ + 1).arg(end).arg(total));
  }

  if (offset_ == 0) {
    prev_page_button_->setDisabled(true);
    first_page_button_->setDisabled(true);
  } else {
    prev_page_button_->setEnabled(true);
    first_page_button_->setEnabled(true);
  }

  if (end == total) {
    next_page_button_->setDisabled(true);
    last_page_button_->setDisabled(true);
  } else {
    next_page_button_->setEnabled(true);
    last_page_button_->setEnabled(true);
  }
}

void DocListButtons::update_top_row_buttons(int n_selected, int n_rows,
                                            int total_n_docs) {
  select_all_button_->setEnabled(n_rows > 0);
  delete_button_->setEnabled(n_selected > 0);
  delete_all_button_->setEnabled(total_n_docs > 0);
  annotate_button_->setEnabled(n_selected == 1);
}

void DocListButtons::update_filter() {
  auto prev_filter = current_filter_;
  auto prev_label_id = current_label_id_;
  auto data = filter_choice_->currentData().value<QPair<int, int>>();
  current_label_id_ = data.first;
  current_filter_ = static_cast<DocListModel::DocFilter>(data.second);
  if ((current_label_id_ != prev_label_id) ||
      (current_filter_ != prev_filter)) {
    offset_ = 0;
    emit doc_filter_changed(current_filter_, current_label_id_, page_size_,
                            offset_);
  }
}

void DocListButtons::setModel(DocListModel* new_model) {
  assert(new_model != nullptr);
  model_ = new_model;
  QObject::connect(this, &DocListButtons::doc_filter_changed, model_,
                   &DocListModel::adjust_query);
  QObject::connect(model_, &DocListModel::modelReset, this,
                   &DocListButtons::update_after_data_change);
  QObject::connect(model_, &DocListModel::database_changed, this,
                   &DocListButtons::after_database_change);
  QObject::connect(model_, &DocListModel::labels_changed, this,
                   &DocListButtons::fill_filter_choice);
  fill_filter_choice();
  update_button_states();
}

DocList::DocList(QWidget* parent) : QFrame(parent) {

  auto layout = new QVBoxLayout();
  setLayout(layout);

  buttons_frame_ = new DocListButtons();
  layout->addWidget(buttons_frame_);

  doc_view_ = new QListView();
  layout->addWidget(doc_view_);
  doc_view_->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QObject::connect(buttons_frame_, &DocListButtons::select_all, doc_view_,
                   &QListView::selectAll);

  QObject::connect(buttons_frame_, &DocListButtons::delete_selected_rows, this,
                   &DocList::delete_selected_rows);

  QObject::connect(buttons_frame_, &DocListButtons::delete_all_docs, this,
                   &DocList::delete_all_docs);

  QObject::connect(buttons_frame_, &DocListButtons::visit_doc, this,
                   [=]() { this->visit_doc(); });

  QObject::connect(doc_view_, &QListView::doubleClicked, this,
                   &DocList::visit_doc);
}

void DocList::setModel(DocListModel* new_model) {
  assert(new_model != nullptr);
  buttons_frame_->setModel(new_model);
  doc_view_->setModel(new_model);
  model_ = new_model;
  QObject::connect(doc_view_->selectionModel(),
                   &QItemSelectionModel::selectionChanged, this,
                   &DocList::update_select_delete_buttons);
  QObject::connect(model_, &DocListModel::modelReset, this,
                   &DocList::update_select_delete_buttons);
  update_select_delete_buttons();
}

int DocList::n_selected_docs() const {
  return doc_view_->selectionModel()->selectedIndexes().size();
}

void DocList::showEvent(QShowEvent* event) {
  if (model_ != nullptr) {
    model_->refresh_current_query_if_outdated();
  }
  QFrame::showEvent(event);
}

void DocList::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::InsertParagraphSeparator)) {
    visit_doc();
    return;
  }
}

void DocList::delete_all_docs() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  int resp = QMessageBox::question(this, "labelbuddy",
                                   QString("Really delete ALL documents?"),
                                   QMessageBox::Ok | QMessageBox::Cancel);
  if (resp != QMessageBox::Ok) {
    return;
  }
  int n_deleted{};
  {
    QProgressDialog progress("Deleting documents...", "Stop", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(delete_docs_dialog_min_duration_ms_);
    n_deleted = model_->delete_all_docs(&progress);
  }
  doc_view_->reset();
  QMessageBox::information(this, "labelbuddy",
                           QString("Deleted %0 document%1")
                               .arg(n_deleted)
                               .arg(n_deleted > 1 ? "s" : ""),
                           QMessageBox::Ok);
}

void DocList::delete_selected_rows() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  QModelIndexList selected = doc_view_->selectionModel()->selectedIndexes();
  if (selected.length() == 0) {
    return;
  }
  int resp = QMessageBox::question(this, "labelbuddy",
                                   QString("Really delete selected documents?"),
                                   QMessageBox::Ok | QMessageBox::Cancel);
  if (resp != QMessageBox::Ok) {
    return;
  }
  int n_before = model_->total_n_docs(DocListModel::DocFilter::all);
  model_->delete_docs(selected);
  int n_after = model_->total_n_docs(DocListModel::DocFilter::all);
  int n_deleted = n_before - n_after;
  QMessageBox::information(this, "labelbuddy",
                           QString("Deleted %0 document%1")
                               .arg(n_deleted)
                               .arg(n_deleted > 1 ? "s" : ""),
                           QMessageBox::Ok);
  doc_view_->reset();
}

void DocList::visit_doc(const QModelIndex& index) {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  if (index.isValid() && index.column() == 0) {
    auto doc_id = model_->data(index, Roles::RowIdRole).toInt();
    emit visit_doc_requested(doc_id);
    return;
  }
  auto selected = doc_view_->selectionModel()->selectedIndexes();
  // items outside of first (visible) column shouldn't be selectable
  assert(selected.empty() || selected[0].column() == 0);
  if (selected.size() != 1 || selected[0].column() != 0) {
    return;
  }
  auto doc_id = model_->data(selected[0], Roles::RowIdRole).toInt();
  assert(doc_id != -1);
  emit visit_doc_requested(doc_id);
}

void DocList::update_select_delete_buttons() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  auto selected = doc_view_->selectionModel()->selectedIndexes();
  int n_rows{};
  for (const auto& sel : selected) {
    if (sel.column() == 0) {
      ++n_rows;
    } else {
      assert(false);
    }
  }
  buttons_frame_->update_top_row_buttons(n_rows, model_->rowCount(),
                                         model_->total_n_docs());
  emit n_selected_docs_changed(n_rows);
}
} // namespace labelbuddy
