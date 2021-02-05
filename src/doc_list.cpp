#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QString>
#include <QVBoxLayout>

#include "doc_list.h"
#include "user_roles.h"
#include "utils.h"

namespace labelbuddy {
DocListButtons::DocListButtons(QWidget* parent) : QFrame(parent) {
  QVBoxLayout* main_layout = new QVBoxLayout();
  QHBoxLayout* buttons_layout = new QHBoxLayout();
  QHBoxLayout* filters_layout = new QHBoxLayout();
  QHBoxLayout* nav_layout = new QHBoxLayout();

  select_all_button = new QPushButton("Select all");
  delete_button = new QPushButton("Delete");
  delete_all_button = new QPushButton("Delete ALL docs");
  annotate_button = new QPushButton("Annotate selected doc");
  buttons_layout->addWidget(select_all_button);
  buttons_layout->addWidget(delete_button);
  buttons_layout->addWidget(delete_all_button);
  buttons_layout->addWidget(annotate_button);

  all_docs_button = new QRadioButton("All docs");
  labelled_docs_button = new QRadioButton("Labelled docs");
  unlabelled_docs_button = new QRadioButton("Unlabelled docs");
  filters_layout->addWidget(all_docs_button);
  filters_layout->addWidget(labelled_docs_button);
  filters_layout->addWidget(unlabelled_docs_button);

  first_page_button = new QPushButton("<<");
  prev_page_button = new QPushButton("<");
  current_page_label = new QLabel();
  current_page_label->setAlignment(Qt::AlignHCenter);
  next_page_button = new QPushButton(">");
  last_page_button = new QPushButton(">>");
  nav_layout->addWidget(first_page_button);
  nav_layout->addWidget(prev_page_button);
  nav_layout->addWidget(current_page_label);
  nav_layout->addWidget(next_page_button);
  nav_layout->addWidget(last_page_button);

  main_layout->addLayout(buttons_layout);
  main_layout->addLayout(filters_layout);
  main_layout->addLayout(nav_layout);
  setLayout(main_layout);

  QObject::connect(labelled_docs_button, &QRadioButton::toggled, this,
                   &DocListButtons::filter_keep_labelled_docs);

  QObject::connect(unlabelled_docs_button, &QRadioButton::toggled, this,
                   &DocListButtons::filter_keep_unlabelled_docs);

  QObject::connect(all_docs_button, &QRadioButton::toggled, this,
                   &DocListButtons::filter_keep_all_docs);

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

  QObject::connect(delete_button, SIGNAL(clicked()), this,
                   SIGNAL(delete_selected_rows()));

  QObject::connect(delete_all_button, &QPushButton::clicked, this,
                   &DocListButtons::delete_all_docs);

  QObject::connect(annotate_button, &QPushButton::clicked, this,
                   &DocListButtons::visit_doc);

  update_button_states();
  all_docs_button->setChecked(true);
}

void DocListButtons::go_to_next_page() {
  if (model == nullptr) {
    return;
  }
  int total = model->total_n_docs(current_filter);
  if (offset + page_size >= total) {
    return;
  }
  offset += page_size;
  emit doc_filter_changed(current_filter, page_size, offset);
}

void DocListButtons::go_to_prev_page() {
  if (model == nullptr) {
    return;
  }
  if (offset == 0) {
    return;
  }
  offset = std::max(0, offset - page_size);
  emit doc_filter_changed(current_filter, page_size, offset);
}

void DocListButtons::go_to_last_page() {
  if (model == nullptr) {
    return;
  }
  int total = model->total_n_docs(current_filter);
  offset = total - total % page_size;
  emit doc_filter_changed(current_filter, page_size, offset);
}

void DocListButtons::go_to_first_page() {
  if (model == nullptr) {
    return;
  }
  offset = 0;
  emit doc_filter_changed(current_filter, page_size, offset);
}

void DocListButtons::update_after_data_change() {
  int total = model->total_n_docs(current_filter);
  int prev_offset = offset;
  offset = std::max(0, std::min(total - 1, offset));
  offset = offset - offset % page_size;
  if (prev_offset != offset) {
    emit doc_filter_changed(current_filter, page_size, offset);
  }
  update_button_states();
}

void DocListButtons::update_button_states() {
  if (model == nullptr) {
    return;
  }

  int total = model->total_n_docs(current_filter);
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

void DocListButtons::filter_keep_labelled_docs(bool checked) {
  if (current_filter == DocListModel::DocFilter::labelled || (!checked)) {
    return;
  }
  current_filter = DocListModel::DocFilter::labelled;
  offset = 0;
  emit doc_filter_changed(current_filter, page_size, offset);
}

void DocListButtons::filter_keep_unlabelled_docs(bool checked) {

  if (current_filter == DocListModel::DocFilter::unlabelled || !checked) {
    return;
  }
  current_filter = DocListModel::DocFilter::unlabelled;
  offset = 0;
  emit doc_filter_changed(current_filter, page_size, offset);
}

void DocListButtons::filter_keep_all_docs(bool checked) {
  if (current_filter == DocListModel::DocFilter::all || !checked) {
    return;
  }
  current_filter = DocListModel::DocFilter::all;
  offset = 0;
  emit doc_filter_changed(current_filter, page_size, offset);
}

void DocListButtons::setModel(DocListModel* new_model) {
  model = new_model;
  QObject::connect(this, &DocListButtons::doc_filter_changed, model,
                   &DocListModel::adjust_query);
  QObject::connect(model, &DocListModel::modelReset, this,
                   &DocListButtons::update_after_data_change);
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
                   [=](){this->visit_doc();});

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
  int n_before = model->total_n_docs(DocListModel::DocFilter::all);
  model->delete_all_docs();
  doc_view->reset();
  QMessageBox::information(this, "labelbuddy",
                           QString("Deleted %0 document%1")
                               .arg(n_before)
                               .arg(n_before > 1 ? "s" : ""),
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
  QModelIndex selected_index;
  if (index.isValid()) {
    selected_index = index;
  } else {
    auto selected = doc_view->selectionModel()->selectedIndexes();
    if (selected.length() == 0) {
      return;
    }
    selected_index = selected[0];
  }
  auto doc_id = model->data(selected_index, Roles::RowIdRole).toInt();
  emit visit_doc_requested(doc_id);
}

void DocList::update_select_delete_buttons() {
  if (model == nullptr) {
    return;
  }
  auto selected = doc_view->selectionModel()->selectedIndexes();
  buttons_frame->update_top_row_buttons(selected.length(), model->rowCount(),
                                        model->total_n_docs());
}
} // namespace labelbuddy
