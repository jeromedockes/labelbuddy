#ifndef LABELBUDDY_DATASET_MENU_H
#define LABELBUDDY_DATASET_MENU_H

#include <QCloseEvent>
#include <QSplitter>

#include "doc_list.h"
#include "doc_list_model.h"
#include "label_list.h"
#include "label_list_model.h"

/// \file
/// Implementation of the Dataset tab (note: has been renamed "Labels & Documents")

namespace labelbuddy {

/// Implementation of the Dataset tab (note: has been renamed "Labels & Documents")
class DatasetMenu : public QSplitter {

  Q_OBJECT

public:
  DatasetMenu(QWidget* parent = nullptr);
  void setDocListModel(DocListModel*);
  void setLabelListModel(LabelListModel*);
  int nSelectedDocs() const;

public slots:
  void storeState();

signals:
  /// User asked to annotate doc with `id` (in the db) `docId`
  void visitDocRequested(int docId);
  void nSelectedDocsChanged(int nDocs);

private:
  LabelList* labelList_;
  DocList* docList_;
  LabelListModel* labelListModel_ = nullptr;
  DocListModel* docListModel_ = nullptr;
};
} // namespace labelbuddy
#endif
