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

  labelList_ = new LabelList();
  addWidget(labelList_);
  scaleMargin(*labelList_, Side::Right);

  docList_ = new DocList();
  addWidget(docList_);
  scaleMargin(*docList_, Side::Left);

  QSettings settings("labelbuddy", "labelbuddy");
  if (settings.contains("DatasetMenu/state")) {
    restoreState(settings.value("DatasetMenu/state").toByteArray());
  }

  QObject::connect(docList_, &DocList::visitDocRequested, this,
                   &DatasetMenu::visitDocRequested);
  QObject::connect(docList_, &DocList::nSelectedDocsChanged, this,
                   &DatasetMenu::nSelectedDocsChanged);
}

void DatasetMenu::connectLabelsAndDocsModels() {
  if (labelListModel_ == nullptr || docListModel_ == nullptr) {
    return;
  }
  QObject::connect(labelListModel_, &LabelListModel::labelsDeleted,
                   docListModel_, &DocListModel::refreshCurrentQuery);
  QObject::connect(labelListModel_, &LabelListModel::labelRenamed,
                   docListModel_, &DocListModel::labelsChanged);
  QObject::connect(labelListModel_, &LabelListModel::labelsAdded, docListModel_,
                   &DocListModel::labelsChanged);
  QObject::connect(labelListModel_, &LabelListModel::labelsDeleted,
                   docListModel_, &DocListModel::labelsChanged);
  QObject::connect(labelListModel_, &LabelListModel::labelsOrderChanged,
                   docListModel_, &DocListModel::labelsChanged);
}

void DatasetMenu::setDocListModel(DocListModel* newModel) {
  assert(newModel != nullptr);
  docListModel_ = newModel;
  docList_->setModel(newModel);
  connectLabelsAndDocsModels();
}

void DatasetMenu::setLabelListModel(LabelListModel* newModel) {
  assert(newModel != nullptr);
  labelListModel_ = newModel;
  labelList_->setModel(newModel);
  connectLabelsAndDocsModels();
}

int DatasetMenu::nSelectedDocs() const { return docList_->nSelectedDocs(); }

void DatasetMenu::storeState() {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("DatasetMenu/state", saveState());
}

} // namespace labelbuddy
