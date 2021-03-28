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
  QVBoxLayout* main_layout = new QVBoxLayout();
  setLayout(main_layout);
  QHBoxLayout* buttons_layout = new QHBoxLayout();
  main_layout->addLayout(buttons_layout);
  QHBoxLayout* filters_layout = new QHBoxLayout();
  main_layout->addLayout(filters_layout);
  QHBoxLayout* nav_layout = new QHBoxLayout();
  main_layout->addLayout(nav_layout);

  select_all_button = new QPushButton("Select all");
  buttons_layout->addWidget(select_all_button);
  delete_button = new QPushButton("Delete");
  buttons_layout->addWidget(delete_button);
  delete_all_button = new QPushButton("Delete all docs");
  buttons_layout->addWidget(delete_all_button);
  annotate_button = new QPushButton("Annotate selected doc");
  buttons_layout->addWidget(annotate_button);

  filters_layout->addWidget(new QLabel("Filter by label: "));
  filter_choice_ = new QComboBox();
  filters_layout->addWidget(filter_choice_);
  filters_layout->addStretch();

  first_page_button = new QPushButton(
      QIcon::fromTheme("go-first", QIcon(":data/icons/go-first.png")), "");
  nav_layout->addWidget(first_page_button);
  prev_page_button = new QPushButton(
      QIcon::fromTheme("go-previous", QIcon(":data/icons/go-previous.png")),
      "");
  nav_layout->addWidget(prev_page_button);
  current_page_label = new QLabel();
  nav_layout->addWidget(current_page_label);
  current_page_label->setAlignment(Qt::AlignHCenter);
  next_page_button = new QPushButton(
      QIcon::fromTheme("go-next", QIcon(":data/icons/go-next.png")), "");
  nav_layout->addWidget(next_page_button);
  last_page_button = new QPushButton(
      QIcon::fromTheme("go-last", QIcon(":data/icons/go-last.png")), "");
  nav_layout->addWidget(last_page_button);

  QObject::connect(next_page_button, &QPushButton::clicked, this,
                   &DocListButtons::go_to_next_page);

  QObject::connect(prev_page_button, &QPushButton::clicked, this,
                   &DocListButtons::go_to_prev_page);

  QObject::connect(first_page_button, &QPushButton::clicked, this,
                   &DocListButtons::go_to_first_page);

  QObject::connect(last_page_button, &QPushButton::clicked, this,
                   &DocListButtons::go_to_last_page);

  QObject::connect(select_all_button, &QPushButton::clicked, this,
                   &DocListButtons::select_all);

  QObject::connect(delete_button, &QPushButton::clicked, this,
                   &DocListButtons::delete_selected_rows);

  QObject::connect(delete_all_button, &QPushButton::clicked, this,
                   &DocListButtons::delete_all_docs);

  QObject::connect(annotate_button, &QPushButton::clicked, this,
                   &DocListButtons::visit_doc);

  void (QComboBox::*combobox_activated)(int) = &QComboBox::activated;
  QObject::connect(filter_choice_, combobox_activated, this,
                   &DocListButtons::update_filter);
  // note above could be simplified: QOverload<int>::of(&QComboBox::activated)
  // but QOverload introduced in qt 5.7 and xenial comes with 5.5

  update_button_states();
}

void DocListButtons::fill_filter_choice() {
  if (model == nullptr) {
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
  auto label_names = model->get_label_names();
  if (label_names.size()) {
    filter_choice_->insertSeparator(filter_choice_->count());
  }
  for (const auto& label_info : label_names) {
    var.setValue(QPair<int, int>(
        label_info.second,
        static_cast<int>(DocListModel::DocFilter::has_given_label)));
    filter_choice_->addItem(label_info.first, var);
  }
  if (label_names.size()) {
    filter_choice_->insertSeparator(filter_choice_->count());
  }
  for (const auto& label_info : label_names) {
    var.setValue(QPair<int, int>(
        label_info.second,
        static_cast<int>(DocListModel::DocFilter::not_has_given_label)));
    filter_choice_->addItem(QString("NOT  %0").arg(label_info.first), var);
  }
  int label_idx{-1};
  switch (current_filter) {
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
        QPair<int, int>(current_label_id, static_cast<int>(current_filter)));
    label_idx = filter_choice_->findData(var);
    if (label_idx != -1) {
      filter_choice_->setCurrentIndex(label_idx);
    } else {
      filter_choice_->setCurrentIndex(0);
    }
    break;
  default:
    filter_choice_->setCurrentIndex(0);
    break;
  }
  update_filter();
}

void DocListButtons::after_database_change() {
  current_filter = DocListModel::DocFilter::all;
  current_label_id = -1;
  offset = 0;
}

void DocListButtons::go_to_next_page() {
  if (model == nullptr) {
    return;
  }
  int total = model->total_n_docs(current_filter, current_label_id);
  if (offset + page_size >= total) {
    return;
  }
  offset += page_size;
  emit doc_filter_changed(current_filter, current_label_id, page_size, offset);
}

void DocListButtons::go_to_prev_page() {
  if (model == nullptr) {
    return;
  }
  if (offset == 0) {
    return;
  }
  offset = std::max(0, offset - page_size);
  emit doc_filter_changed(current_filter, current_label_id, page_size, offset);
}

void DocListButtons::go_to_last_page() {
  if (model == nullptr) {
    return;
  }
  int total = model->total_n_docs(current_filter, current_label_id);
  offset = total - total % page_size;
  emit doc_filter_changed(current_filter, current_label_id, page_size, offset);
}

void DocListButtons::go_to_first_page() {
  if (model == nullptr) {
    return;
  }
  offset = 0;
  emit doc_filter_changed(current_filter, current_label_id, page_size, offset);
}

void DocListButtons::update_after_data_change() {
  int total = model->total_n_docs(current_filter, current_label_id);
  int prev_offset = offset;
  offset = std::max(0, std::min(total - 1, offset));
  offset = offset - offset % page_size;
  if (prev_offset != offset) {
    emit doc_filter_changed(current_filter, current_label_id, page_size,
                            offset);
  }
  update_button_states();
}

void DocListButtons::update_button_states() {
  if (model == nullptr) {
    return;
  }

  int total = model->total_n_docs(current_filter, current_label_id);
  int end = offset + model->rowCount();

  if (total == 0) {
    current_page_label->setText("0 / 0");
  } else if (offset == end - 1) {
    current_page_label->setText(QString("%1 / %2").arg(end).arg(total));
  } else {
    current_page_label->setText(
        QString("%1 - %2 / %3").arg(offset + 1).arg(end).arg(total));
  }

  if (offset == 0) {
    prev_page_button->setDisabled(true);
    first_page_button->setDisabled(true);
  } else {
    prev_page_button->setEnabled(true);
    first_page_button->setEnabled(true);
  }

  if (end == total) {
    next_page_button->setDisabled(true);
    last_page_button->setDisabled(true);
  } else {
    next_page_button->setEnabled(true);
    last_page_button->setEnabled(true);
  }
}

void DocListButtons::update_top_row_buttons(int n_selected, int n_rows,
                                            int total_n_docs) {
  select_all_button->setEnabled(n_rows > 0);
  delete_button->setEnabled(n_selected > 0);
  delete_all_button->setEnabled(total_n_docs > 0);
  annotate_button->setEnabled(n_selected == 1);
}

void DocListButtons::update_filter() {
  auto prev_filter = current_filter;
  auto prev_label_id = current_label_id;
  auto data = filter_choice_->currentData().value<QPair<int, int>>();
  current_label_id = data.first;
  current_filter = static_cast<DocListModel::DocFilter>(data.second);
  if ((current_label_id != prev_label_id) || (current_filter != prev_filter)) {
    offset = 0;
    emit doc_filter_changed(current_filter, current_label_id, page_size,
                            offset);
  }
}

void DocListButtons::setModel(DocListModel* new_model) {
  model = new_model;
  QObject::connect(this, &DocListButtons::doc_filter_changed, model,
                   &DocListModel::adjust_query);
  QObject::connect(model, &DocListModel::modelReset, this,
                   &DocListButtons::update_after_data_change);
  QObject::connect(model, &DocListModel::database_changed, this,
                   &DocListButtons::after_database_change);
  QObject::connect(model, &DocListModel::labels_changed, this,
                   &DocListButtons::fill_filter_choice);
  fill_filter_choice();
  update_button_states();
}

DocList::DocList(QWidget* parent) : QFrame(parent) {

  QVBoxLayout* layout = new QVBoxLayout();
  setLayout(layout);

  buttons_frame = new DocListButtons();
  layout->addWidget(buttons_frame);

  doc_view = new QListView();
  layout->addWidget(doc_view);
  doc_view->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QObject::connect(buttons_frame, &DocListButtons::select_all, doc_view,
                   &QListView::selectAll);

  QObject::connect(buttons_frame, &DocListButtons::delete_selected_rows, this,
                   &DocList::delete_selected_rows);

  QObject::connect(buttons_frame, &DocListButtons::delete_all_docs, this,
                   &DocList::delete_all_docs);

  QObject::connect(buttons_frame, &DocListButtons::visit_doc, this,
                   [=]() { this->visit_doc(); });

  QObject::connect(doc_view, &QListView::doubleClicked, this,
                   &DocList::visit_doc);
}

void DocList::setModel(DocListModel* new_model) {

  buttons_frame->setModel(new_model);
  doc_view->setModel(new_model);
  model = new_model;
  QObject::connect(doc_view->selectionModel(),
                   &QItemSelectionModel::selectionChanged, this,
                   &DocList::update_select_delete_buttons);
  QObject::connect(model, &DocListModel::modelReset, this,
                   &DocList::update_select_delete_buttons);
  update_select_delete_buttons();
}

void DocList::showEvent(QShowEvent* event) {
  if (model != nullptr) {
    model->refresh_current_query_if_outdated();
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
  if (model == nullptr) {
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
    progress.setMinimumDuration(2000);
    n_deleted = model->delete_all_docs(&progress);
  }
  doc_view->reset();
  QMessageBox::information(this, "labelbuddy",
                           QString("Deleted %0 document%1")
                               .arg(n_deleted)
                               .arg(n_deleted > 1 ? "s" : ""),
                           QMessageBox::Ok);
}

void DocList::delete_selected_rows() {
  if (model == nullptr) {
    return;
  }
  QModelIndexList selected = doc_view->selectionModel()->selectedIndexes();
  if (selected.length() == 0) {
    return;
  }
  int resp = QMessageBox::question(this, "labelbuddy",
                                   QString("Really delete selected documents?"),
                                   QMessageBox::Ok | QMessageBox::Cancel);
  if (resp != QMessageBox::Ok) {
    return;
  }
  int n_before = model->total_n_docs(DocListModel::DocFilter::all);
  model->delete_docs(selected);
  int n_after = model->total_n_docs(DocListModel::DocFilter::all);
  int n_deleted = n_before - n_after;
  QMessageBox::information(this, "labelbuddy",
                           QString("Deleted %0 document%1")
                               .arg(n_deleted)
                               .arg(n_deleted > 1 ? "s" : ""),
                           QMessageBox::Ok);
  doc_view->reset();
}

void DocList::visit_doc(const QModelIndex& index) {
  if (model == nullptr) {
    return;
  }
  if (index.isValid() && index.column() == 0) {
    auto doc_id = model->data(index, Roles::RowIdRole).toInt();
    emit visit_doc_requested(doc_id);
    return;
  }
  auto selected = doc_view->selectionModel()->selectedIndexes();
  if (selected.size() != 1 || selected[0].column() != 0) {
    return;
  }
  auto doc_id = model->data(selected[0], Roles::RowIdRole).toInt();
  emit visit_doc_requested(doc_id);
}

void DocList::update_select_delete_buttons() {
  if (model == nullptr) {
    return;
  }
  auto selected = doc_view->selectionModel()->selectedIndexes();
  int n_rows{};
  for (const auto& sel : selected) {
    if (sel.column() == 0) {
      ++n_rows;
    }
  }
  buttons_frame->update_top_row_buttons(n_rows, model->rowCount(),
                                        model->total_n_docs());
}
} // namespace labelbuddy
