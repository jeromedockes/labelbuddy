#ifndef LABELBUDDY_DATASET_MENU_H
#define LABELBUDDY_DATASET_MENU_H

#include <QSplitter>
#include <QCloseEvent>

#include "doc_list.h"
#include "doc_list_model.h"
#include "label_list.h"
#include "label_list_model.h"

namespace labelbuddy {
class DatasetMenu : public QSplitter {

  Q_OBJECT

public:
  DatasetMenu(QWidget* parent = nullptr);
  void set_doc_list_model(DocListModel*);
  void set_label_list_model(LabelListModel*);

public slots:
  void store_state();

signals:
  void visit_doc_requested(int doc_id);

private:
  LabelList* label_list;
  DocList* doc_list;
  LabelListModel* label_list_model = nullptr;
  DocListModel* doc_list_model = nullptr;
};
} // namespace labelbuddy
#endif
