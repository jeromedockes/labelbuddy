#ifndef LABELBUDDY_ANNOTATIONS_MODEL_H
#define LABELBUDDY_ANNOTATIONS_MODEL_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QSqlQuery>
#include <QString>

/// \file
/// Model providing information to the Annotator

namespace labelbuddy {

struct LabelInfo {
  int id;
  QString color;
};

struct AnnotationInfo {
  int id;
  int label_id;
  int start_char;
  int end_char;
};

/// Model providing information to the Annotator

/// It is positionned on one particular document and provides information such
/// as its text and annotations.
/// Can be moved to a different document using `visit_next`,
/// `visit_next_labelled`, `visit_doc` etc.
class AnnotationsModel : public QObject {
  Q_OBJECT

public:
  AnnotationsModel(QObject* parent = nullptr);

  /// Get the `content` (text) of the current document.

  /// Returns the empty string
  /// if current doc not in database (happens for example if database is empty).
  QString get_content() const;

  /// title for current document

  /// return:
  /// - if it exists, `short_title`
  /// - otherwise if it exists, `title`
  /// - otherwise the empty string
  QString get_title() const;

  /// Insert an annotation for the current doc.

  /// (does not check for overlap with other annotations) If insertion fails (eg
  /// due to db violation constraint) returns -1 Otherwise returns the new
  /// annotation's `rowid`. If this is the document's first annotation, emits
  /// `document_status_changed` (it changed from unlabelled to labelled)
  int add_annotation(int label_id, int start_char, int end_char);

  /// Delete annotations provided their `rowid` in the database

  /// If after delete operation the current doc has 0 annotations, emits
  /// `document_status_changed` (as it changed from labelled to unlabelled).
  ///
  /// Also emits `annotations_changed`
  void delete_annotations(QList<int>);

  /// Info for all labels in the database
  QMap<int, LabelInfo> get_labels_info() const;

  /// Info for annotations on the current document.
  QMap<int, AnnotationInfo> get_annotations_info() const;

  /// Current doc's position in the list of all docs sorted by id

  /// Note this is not the same as the id
  int current_doc_position() const;

  /// number of docs in the database
  int total_n_docs() const;
  bool has_next() const;
  bool has_prev() const;
  bool has_next_labelled() const;
  bool has_prev_labelled() const;
  bool has_next_unlabelled() const;
  bool has_prev_unlabelled() const;

  /// get the `id` of label that has shortcut key `shortcut`

  /// returns -1 if no label has that shortcut.
  int shortcut_to_id(const QString& shortcut) const;

public slots:

  void visit_next();
  void visit_prev();
  void visit_next_labelled();
  void visit_prev_labelled();
  void visit_next_unlabelled();
  void visit_prev_unlabelled();
  void visit_doc(int doc_id);
  void visit_first_doc();

  /// check that the current doc still exists and go to first doc if not

  /// called after delete operations in the Dataset tab
  void check_current_doc();

  void set_database(const QString& new_database_name);

signals:
  void annotations_changed();

  /// current document changed, ie we are now visiting a different doc
  void document_changed();

  /// current document's status (labelled or unlabelled) changed
  void document_status_changed();

private:
  int current_doc_id = -1;
  QString database_name;

  QSqlQuery get_query() const;
  bool visit_query_result(const QString& query_text);
  int get_query_result(const QString& query_text) const;

  int last_doc_id() const;
  int first_doc_id() const;
  int last_labelled_doc_id() const;
  int first_labelled_doc_id() const;
  int last_unlabelled_doc_id() const;
  int first_unlabelled_doc_id() const;

};
} // namespace labelbuddy

#endif
