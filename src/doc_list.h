#ifndef LABELBUDDY_DOC_LIST_H
#define LABELBUDDY_DOC_LIST_H

#include <QFrame>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QRadioButton>
#include <QSqlDatabase>

#include "doc_list_model.h"

namespace labelbuddy {
class DocListButtons : public QFrame {
  Q_OBJECT

public:
  DocListButtons(QWidget* parent = nullptr);
  void setModel(DocListModel*);

public slots:

  void filter_keep_labelled_docs(bool checked);

  void filter_keep_unlabelled_docs(bool checked);

  void filter_keep_all_docs(bool checked);

  void update_button_states();

  void update_after_data_change();

  void go_to_next_page();

  void go_to_last_page();

  void go_to_prev_page();

  void go_to_first_page();

  void update_top_row_buttons(int n_selected, int n_rows, int total_n_docs);

signals:

  void doc_filter_changed(DocListModel::DocFilter, int limit, int offset);
  void select_all();
  void delete_selected_rows();
  void delete_all_docs();
  void visit_doc();

private:
  int offset = 0;
  int page_size = 100;
  DocListModel::DocFilter current_filter = DocListModel::DocFilter::all;
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

  QRadioButton* all_docs_button = nullptr;
  QRadioButton* labelled_docs_button = nullptr;
  QRadioButton* unlabelled_docs_button = nullptr;
};

class DocList : public QFrame {
  Q_OBJECT
public:
  DocList(QWidget* parent = nullptr);

  void setModel(DocListModel*);

public slots:

  void delete_selected_rows();
  void delete_all_docs();
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
