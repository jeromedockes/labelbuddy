#ifndef LABELBUDDY_DOC_LIST_H
#define LABELBUDDY_DOC_LIST_H

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QShowEvent>
#include <QSqlDatabase>

#include "doc_list_model.h"

/// \file
/// document list in the Dataset tab

namespace labelbuddy {

/// Set of buttons to navigate the doc list or delete documents
class DocListButtons : public QFrame {
  Q_OBJECT

public:
  DocListButtons(QWidget* parent = nullptr);
  void setModel(DocListModel*);

  /// Update the state of the "delete" and "annotate" buttons
  /// \param n_selected number of docs selected in the doc list view
  /// \param n_rows number of rows in the doc list view
  /// \param total_n_docs total number of documents in the database
  void update_top_row_buttons(int n_selected, int n_rows, int total_n_docs);

signals:

  void doc_filter_changed(DocListModel::DocFilter, int filter_label_id,
                          int limit, int offset);
  void select_all();
  void delete_selected_rows();
  void delete_all_docs();

  /// User asked to see selected document in Annotate tab
  void visit_doc();

private slots:

  /// prepare the combobox

  /// the items' `itemData` is a pair (label id, doc filter)
  void fill_filter_choice();

  /// reset filter and offset when database changes
  void after_database_change();

  void update_button_states();

  /// adjust offset and button states when model data changes
  void update_after_data_change();

  void go_to_next_page();

  void go_to_last_page();

  void go_to_prev_page();

  void go_to_first_page();

  /// adjust doc filter when combobox selection changes
  void update_filter();

private:
  int offset = 0;
  int page_size = 100;
  DocListModel::DocFilter current_filter = DocListModel::DocFilter::all;
  // label used to either include or exclude docs
  int current_label_id = -1;
  DocListModel* model = nullptr;
  QLabel* current_page_label = nullptr;

  QPushButton* select_all_button = nullptr;
  QPushButton* delete_button = nullptr;
  QPushButton* delete_all_button = nullptr;
  QPushButton* annotate_button = nullptr;

  QPushButton* prev_page_button = nullptr;
  QPushButton* next_page_button = nullptr;
  QPushButton* first_page_button = nullptr;
  QPushButton* last_page_button = nullptr;

  QComboBox* filter_choice_ = nullptr;

  void add_connections();
};

/// document list in the Dataset tab
class DocList : public QFrame {
  Q_OBJECT
public:
  DocList(QWidget* parent = nullptr);

  void setModel(DocListModel*);

protected:
  void showEvent(QShowEvent* event) override;

  void keyPressEvent(QKeyEvent* event) override;

private slots:

  /// ask for confirmation and tell the model to delete selected docs
  void delete_selected_rows();
  /// ask for confirmation and tell the model to delete all docs
  void delete_all_docs();
  /// get the doc's id and emit `visit_doc_requested`
  void visit_doc(const QModelIndex& = QModelIndex());
  void update_select_delete_buttons();

signals:

  void visit_doc_requested(int doc_id);

private:
  DocListButtons* buttons_frame;
  QListView* doc_view;
  DocListModel* model = nullptr;
};
} // namespace labelbuddy

#endif // LABELBUDDY_DOC_LIST_H
