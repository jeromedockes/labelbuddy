#ifndef LABELBUDDY_ANNOTATOR_H
#define LABELBUDDY_ANNOTATOR_H

#include <list>
#include <memory>
#include <set>

#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QSplitter>
#include <QSqlQueryModel>
#include <QWidget>

#include "annotations_model.h"
#include "label_list.h"
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
  /// Set the extra data in the line edit
  void set_extra_data(const QString& new_data);
  bool is_label_choice_enabled() const;

signals:
  void selectionChanged();
  void delete_button_clicked();
  void extra_data_edited(const QString& new_data);
  void extra_data_edit_finished();

public slots:
  void enable_delete_and_edit();
  void disable_delete_and_edit();
  void enable_label_choice();
  void disable_label_choice();

private slots:
  void on_selection_change();
  void on_delete_button_click();

private:
  QPushButton* delete_button;
  NoDeselectAllView* labels_view;
  LabelListModel* label_list_model = nullptr;
  std::unique_ptr<LabelDelegate> label_delegate_ = nullptr;
  QLineEdit* extra_data_edit = nullptr;
  QLabel* extra_data_label = nullptr;
};

/// Navigation buttons above the text: next, next labelled etc.
class AnnotationsNavButtons : public QWidget {
  Q_OBJECT

public:
  AnnotationsNavButtons(QWidget* parent = nullptr);
  void setModel(AnnotationsModel*);

public slots:
  void update_button_states();
  void set_skip_updating(bool skip);

private:
  AnnotationsModel* annotations_model = nullptr;
  QPushButton* prev_labelled_button;
  QPushButton* prev_unlabelled_button;
  QPushButton* prev_button;

  QLabel* current_doc_label;

  QPushButton* next_button;
  QPushButton* next_unlabelled_button;
  QPushButton* next_labelled_button;

  bool skip_updating_buttons_{};

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
  QString extra_data;
  QTextCursor cursor;
};

struct AnnotationIndex {
  int start_char;
  int id;
};

bool operator<(const AnnotationIndex& lhs, const AnnotationIndex& rhs);
bool operator>(const AnnotationIndex& lhs, const AnnotationIndex& rhs);
bool operator<=(const AnnotationIndex& lhs, const AnnotationIndex& rhs);
bool operator>=(const AnnotationIndex& lhs, const AnnotationIndex& rhs);
bool operator==(const AnnotationIndex& lhs, const AnnotationIndex& rhs);
bool operator!=(const AnnotationIndex& lhs, const AnnotationIndex& rhs);

struct Cluster {
  AnnotationIndex first_annotation;
  AnnotationIndex last_annotation;
  int start_char;
  int end_char;
};

struct StatusBarInfo {
  QString doc_info;
  QString annotation_info;
  QString annotation_label;
};

/// The widget that allows creating and manipulating annotations

/// There can be several annotations for the document, highlighted with their
/// label's color. Moreover, one of them can be "active"; it is the one whose
/// label is changed if we use the label list, or that can be deleted, and its
/// text is underlined and shown in bold font (by default).
///
/// Groups of overlapping annotations form "clusters", that are shown in gray
/// with white text except possibly for the active annotation.
class Annotator : public QSplitter {
  Q_OBJECT

public:
  Annotator(QWidget* parent = nullptr);
  void keyPressEvent(QKeyEvent*) override;

  /// remember that mouse was pressed
  void mousePressEvent(QMouseEvent*) override;

  /// update annotations cursorPositionChanged signal hasn't happened since
  /// the last mousePressEvent
  ///
  /// cursorPositionChanged can be due to mouse press but also to keyboard
  /// actions. Moreover, a mouse click on the same position will not trigger a
  /// cursorPositionChanged. Therefore when the mouse is pressed we set
  /// `need_update_active_anno_` to true, and when it is released if it hasn't
  /// been set to false we update the active annotation and repaint annotations.
  void mouseReleaseEvent(QMouseEvent*) override;
  void set_annotations_model(AnnotationsModel*);
  void set_label_list_model(LabelListModel*);

  /// If there is a currently active (selected) annotation return its label's
  /// `id`, otherwise return -1 .
  int active_annotation_label() const;

  /// If there is a currently active annotation return its extra data
  /// "" if there is no active annotation or its data is NULL
  QString active_annotation_extra_data() const;

  StatusBarInfo current_status_info() const;

signals:

  void active_annotation_changed();
  void current_status_display_changed();

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
  void set_font(const QFont& new_font);
  void set_use_bold_font(bool use_bold);

  void reset_skip_updating_nav_buttons();

protected:
  /// Filter installed on the textedit to override the behaviour of Space key
  bool eventFilter(QObject* object, QEvent* event) override;

private slots:
  void set_default_focus();
  void delete_active_annotation();
  void update_extra_data_for_active_annotation(const QString& new_data);
  void activate_cluster_at_cursor_pos();
  void set_label_for_selected_region();
  void update_label_choices_button_states();
  void reset_document();

private:
  void clear_annotations();
  void fetch_labels_info();
  void fetch_annotations_info();
  QTextEdit::ExtraSelection
  make_painted_region(int start_char, int end_char, const QString& color,
                      const QString& text_color = "black",
                      bool underline = false);
  void paint_annotations();
  bool add_annotation();
  bool add_annotation(int label_id, int start_char, int end_char);
  void delete_annotation(int);
  void deactivate_active_annotation();
  std::list<Cluster>::const_iterator cluster_at_pos(int pos) const;

  /// pos: {start_char, id}
  int find_next_annotation(AnnotationIndex pos, bool forward = true) const;

  /// Update clusters with a new annotation.

  /// Clusters that overlap with the new annotation are merged
  void add_annotation_to_clusters(const AnnotationCursor& annotation,
                                  std::list<Cluster>& clusters);

  /// Remove an annotation and update the clusters
  void remove_annotation_from_clusters(const AnnotationCursor& annotation,
                                       std::list<Cluster>& clusters);

  int active_annotation = -1;
  bool need_update_active_anno_{};
  bool active_anno_format_is_set_{};

  /// clusters of overlapping annotations
  std::list<Cluster> clusters_{};
  QMap<int, AnnotationCursor> annotations{};
  QMap<int, LabelInfo> labels{};

  /// Sorting annotations by {start_char, id}
  std::set<AnnotationIndex> sorted_annotations_{};

  QLabel* title_label;
  SearchableText* text;
  LabelChoices* label_choices;
  AnnotationsModel* annotations_model = nullptr;
  AnnotationsNavButtons* nav_buttons = nullptr;
  QTextCharFormat default_format;
  bool use_bold_font{};
};

} // namespace labelbuddy

#endif
