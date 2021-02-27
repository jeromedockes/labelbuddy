#ifndef LABELBUDDY_ANNOTATOR_H
#define LABELBUDDY_ANNOTATOR_H

#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QSplitter>
#include <QSqlQueryModel>
#include <QWidget>

#include "annotations_model.h"
#include "label_list_model.h"
#include "no_deselect_all_view.h"
#include "searchable_text.h"

namespace labelbuddy {

class LabelChoices : public QWidget {
  Q_OBJECT

public:
  LabelChoices(QWidget* parent = nullptr);
  void setModel(LabelListModel*);
  QModelIndexList selectedIndexes() const;
  int selected_label_id() const;
  void set_selected_label_id(int label_id);

signals:
  void selectionChanged();
  void delete_button_clicked();

public slots:
  void on_selection_change();
  void on_delete_button_click();
  void enable_delete();
  void disable_delete();
  void enable_label_choice();
  void disable_label_choice();
  bool is_label_choice_enabled() const;

private:
  QPushButton* delete_button;
  NoDeselectAllView* labels_view;
  LabelListModel* label_list_model = nullptr;
};

class AnnotationsNavButtons : public QWidget {
  Q_OBJECT

public:
  AnnotationsNavButtons(QWidget* parent = nullptr);
  void setModel(AnnotationsModel*);

public slots:
  void update_button_states();

private:
  AnnotationsModel* annotations_model = nullptr;
  QPushButton* prev_labelled_button;
  QPushButton* prev_unlabelled_button;
  QPushButton* prev_button;

  QLabel* current_doc_label;

  QPushButton* next_button;
  QPushButton* next_unlabelled_button;
  QPushButton* next_labelled_button;

signals:
  void visit_next();
  void visit_prev();
  void visit_next_labelled();
  void visit_prev_labelled();
  void visit_next_unlabelled();
  void visit_prev_unlabelled();
};

struct AnnotationCursor {
  int id;
  int label_id;
  int start_char;
  int end_char;
  QTextCursor cursor;
};

class Annotator : public QSplitter {
  Q_OBJECT

public:
  Annotator(QWidget* parent = nullptr);
  void keyPressEvent(QKeyEvent*) override;
  void set_annotations_model(AnnotationsModel*);
  void set_label_list_model(LabelListModel*);
  int annotation_at_pos(int) const;
  int active_annotation_label() const;
  bool is_monospace() const;
  bool is_using_bold() const;

signals:

  void active_annotation_changed();

public slots:

  void delete_active_annotation();
  void activate_annotation_at_cursor_pos();
  void set_label_for_selected_region();
  void update_label_choices_button_states();
  void reset_document();
  void update_annotations();
  void update_nav_buttons();
  void store_state();
  void set_monospace_font(bool monospace = true);
  void set_use_bold_font(bool use_bold = true);
  void select_next_annotation(bool forward = true);

protected:
  bool eventFilter(QObject* object, QEvent* event) override;

private:
  void fetch_labels_info();
  void fetch_annotations_info();
  void paint_annotations();
  void add_annotation();
  void add_annotation(int label_id, int start_char, int end_char);
  void delete_overlapping_annotations(int start_char, int end_char);
  void delete_annotations(QList<int>);
  void delete_annotation(int);
  void deactivate_active_annotation();
  int find_next_annotation(int pos, bool forward = true) const;
  int active_annotation = -1;
  QMap<int, AnnotationCursor> annotations{};
  QMap<int, LabelInfo> labels{};
  QMap<int, int> pos_to_anno{};

  QLabel* title_label;
  SearchableText* text;
  LabelChoices* label_choices;
  AnnotationsModel* annotations_model = nullptr;
  AnnotationsNavButtons* nav_buttons;
  int default_weight;
  bool monospace_font{};
  bool use_bold_font{true};
};

} // namespace labelbuddy

#endif
