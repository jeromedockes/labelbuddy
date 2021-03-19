#include <QSettings>

#include "dataset_menu.h"
#include "doc_list.h"
#include "doc_list_model.h"
#include "label_list.h"
#include "label_list_model.h"
#include "utils.h"

namespace labelbuddy {
DatasetMenu::DatasetMenu(QWidget* parent) : QSplitter(parent) {

  label_list = new LabelList();
  addWidget(label_list);
  scale_margin(*label_list, Side::Right);

  doc_list = new DocList();
  addWidget(doc_list);
  scale_margin(*doc_list, Side::Left);

  QSettings settings("labelbuddy", "labelbuddy");
  if (settings.contains("DatasetMenu/state")) {
    restoreState(settings.value("DatasetMenu/state").toByteArray());
  }

  QObject::connect(doc_list, &DocList::visit_doc_requested, this,
                   &DatasetMenu::visit_doc_requested);
}

void DatasetMenu::set_doc_list_model(DocListModel* new_model) {
  doc_list_model = new_model;
  doc_list->setModel(new_model);
  if (label_list_model != nullptr) {
    QObject::connect(label_list_model, &LabelListModel::labels_deleted,
                     doc_list_model, &DocListModel::refresh_current_query);
  }
}

void DatasetMenu::set_label_list_model(LabelListModel* new_model) {
  label_list_model = new_model;
  label_list->setModel(new_model);
  if (doc_list_model != nullptr) {
    QObject::connect(label_list_model, &LabelListModel::labels_deleted,
                     doc_list_model, &DocListModel::refresh_current_query);
  }
}
void DatasetMenu::store_state() {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("DatasetMenu/state", saveState());
}

} // namespace labelbuddy
