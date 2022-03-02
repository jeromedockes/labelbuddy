#include <cassert>

#include <QSettings>

#include "dataset_menu.h"
#include "doc_list.h"
#include "doc_list_model.h"
#include "label_list.h"
#include "label_list_model.h"
#include "utils.h"

namespace labelbuddy {
DatasetMenu::DatasetMenu(QWidget* parent) : QSplitter(parent) {

  label_list_ = new LabelList();
  addWidget(label_list_);
  scale_margin(*label_list_, Side::Right);

  doc_list_ = new DocList();
  addWidget(doc_list_);
  scale_margin(*doc_list_, Side::Left);

  QSettings settings("labelbuddy", "labelbuddy");
  if (settings.contains("DatasetMenu/state")) {
    restoreState(settings.value("DatasetMenu/state").toByteArray());
  }

  QObject::connect(doc_list_, &DocList::visit_doc_requested, this,
                   &DatasetMenu::visit_doc_requested);
  QObject::connect(doc_list_, &DocList::n_selected_docs_changed, this,
                   &DatasetMenu::n_selected_docs_changed);
}

void DatasetMenu::set_doc_list_model(DocListModel* new_model) {
  assert(new_model != nullptr);
  doc_list_model_ = new_model;
  doc_list_->setModel(new_model);
  if (label_list_model_ != nullptr) {
    QObject::connect(label_list_model_, &LabelListModel::labels_deleted,
                     doc_list_model_, &DocListModel::refresh_current_query);
    QObject::connect(label_list_model_, &LabelListModel::labels_added,
                     doc_list_model_, &DocListModel::labels_changed);
    QObject::connect(label_list_model_, &LabelListModel::labels_deleted,
                     doc_list_model_, &DocListModel::labels_changed);
    QObject::connect(label_list_model_, &LabelListModel::labels_order_changed,
                     doc_list_model_, &DocListModel::labels_changed);
  }
}

void DatasetMenu::set_label_list_model(LabelListModel* new_model) {
  assert(new_model != nullptr);
  label_list_model_ = new_model;
  label_list_->setModel(new_model);
  if (doc_list_model_ != nullptr) {
    QObject::connect(label_list_model_, &LabelListModel::labels_deleted,
                     doc_list_model_, &DocListModel::refresh_current_query);
    QObject::connect(label_list_model_, &LabelListModel::labels_added,
                     doc_list_model_, &DocListModel::labels_changed);
    QObject::connect(label_list_model_, &LabelListModel::labels_deleted,
                     doc_list_model_, &DocListModel::labels_changed);
    QObject::connect(label_list_model_, &LabelListModel::labels_order_changed,
                     doc_list_model_, &DocListModel::labels_changed);
  }
}

int DatasetMenu::n_selected_docs() const { return doc_list_->n_selected_docs(); }

void DatasetMenu::store_state() {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("DatasetMenu/state", saveState());
}

} // namespace labelbuddy
