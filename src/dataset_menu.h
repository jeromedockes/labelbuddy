#ifndef LABELBUDDY_DATASET_MENU_H
#define LABELBUDDY_DATASET_MENU_H

#include <QCloseEvent>
#include <QSplitter>

#include "doc_list.h"
#include "doc_list_model.h"
#include "label_list.h"
#include "label_list_model.h"

/// \file
/// Implementation of the Dataset tab

namespace labelbuddy {

/// Implementation of the Dataset tab
class DatasetMenu : public QSplitter {

  Q_OBJECT

public:
  DatasetMenu(QWidget* parent = nullptr);
  void set_doc_list_model(DocListModel*);
  void set_label_list_model(LabelListModel*);
  int n_selected_docs() const;

public slots:
  void store_state();

signals:
  /// User asked to annotate doc with `id` (in the db) `doc_id`
  void visit_doc_requested(int doc_id);
  void n_selected_docs_changed(int n_docs);

private:
  LabelList* label_list_;
  DocList* doc_list_;
  LabelListModel* label_list_model_ = nullptr;
  DocListModel* doc_list_model_ = nullptr;
};
} // namespace labelbuddy
#endif
