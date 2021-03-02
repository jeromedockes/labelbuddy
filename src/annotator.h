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

/// \file
/// Implementation of the Annotator tab

namespace labelbuddy {

/// Widget use to set the label for the selection
class LabelChoices : public QWidget {
  Q_OBJECT

public:
  LabelChoices(QWidget* parent = nullptr);
  void setModel(LabelListModel*);
  QModelIndexList selectedIndexes() const;
  /// The currently selected label's `id` in the database
  int selected_label_id() const;
  /// Set the currently selected label based on its `id` in the db
  void set_selected_label_id(int label_id);

signals:
  void selectionChanged();
  void delete_button_clicked();

public slots:
  void enable_delete();
  void disable_delete();
  void enable_label_choice();
  void disable_label_choice();
  bool is_label_choice_enabled() const;

private slots:
  void on_selection_change();
  void on_delete_button_click();

private:
  QPushButton* delete_button;
  NoDeselectAllView* labels_view;
  LabelListModel* label_list_model = nullptr;
};

/// Navigation buttons above the text: next, next labelled etc.
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

/// The widget that allows creating and manipulating annotations

/// at the moment annotations are non-overlapping: if we create an annotation
/// that overlaps with a previously existing one the previously existing one is
/// removed.

/// There can be several annotations for the document, highlighted with their
/// label's color. Moreover, one of them can be "active"; it is the one whose
/// label is changed if we use the label list, or that can be deleted, and its
/// text is underlined and shown in bold font (by default).
///
class Annotator : public QSplitter {
  Q_OBJECT

public:
  Annotator(QWidget* parent = nullptr);
  void keyPressEvent(QKeyEvent*) override;
  void set_annotations_model(AnnotationsModel*);
  void set_label_list_model(LabelListModel*);

  /// return the `id` (in the db) of the annotation that contains the character
  /// at position `pos`. if that character is not in an annotation return -1 .
  int annotation_at_pos(int) const;

  /// If there is a currently active (selected) annotation return its label's
  /// `id`, otherwise return -1 .
  int active_annotation_label() const;

  /// Is text displayed with a fixed-width font
  bool is_monospace() const;

  /// Is the active annotation shown with bold text
  bool is_using_bold() const;

signals:

  void active_annotation_changed();

public slots:

  /// Update the list of annotations e.g. after changing the document or changes
  /// to the labels in the database.
  void update_annotations();

  /// Refresh the states of navigation buttons eg if documents or annotations
  /// have been added or removed
  void update_nav_buttons();

  /// jump to next annotation and make it active
  void select_next_annotation(bool forward = true);

  /// Remember window state (slider position) in QSettings
  void store_state();
  void set_monospace_font(bool monospace = true);
  void set_use_bold_font(bool use_bold = true);

protected:
  /// Filter installed on the textedit to override the behaviour of Space key
  bool eventFilter(QObject* object, QEvent* event) override;

private slots:
  void delete_active_annotation();
  void activate_annotation_at_cursor_pos();
  void set_label_for_selected_region();
  void update_label_choices_button_states();
  void reset_document();

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
