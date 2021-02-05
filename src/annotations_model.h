#ifndef LABELBUDDY_ANNOTATIONS_MODEL_H
#define LABELBUDDY_ANNOTATIONS_MODEL_H

#include <QSqlQuery>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

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

class AnnotationsModel : public QObject {
  Q_OBJECT

public:
  AnnotationsModel(QObject* parent = nullptr);
  QString get_content() const;
  int add_annotation(int label_id, int start_char, int end_char);
  void delete_annotations(QList<int>);
  QMap<int, LabelInfo> get_labels_info() const;
  QMap<int, AnnotationInfo> get_annotations_info() const;
  int current_doc_position() const;
  int total_n_docs() const;
  int last_doc_id() const;
  int first_doc_id() const;
  int last_labelled_doc_id() const;
  int first_labelled_doc_id() const;
  int last_unlabelled_doc_id() const;
  int first_unlabelled_doc_id() const;
  bool has_next() const;
  bool has_prev() const;
  bool has_next_labelled() const;
  bool has_prev_labelled() const;
  bool has_next_unlabelled() const;
  bool has_prev_unlabelled() const;

public slots:

  void visit_next();
  void visit_prev();
  void visit_next_labelled();
  void visit_prev_labelled();
  void visit_next_unlabelled();
  void visit_prev_unlabelled();
  void visit_doc(int doc_id);
  void visit_first_doc();
  void check_current_doc();
  void set_database(const QString& new_database_name);

signals:
  void annotations_changed();
  void document_changed();
  void document_status_changed();

private:

  int current_doc_id = -1;
  QString database_name;

  QSqlQuery get_query() const;
  bool visit_query_result(const QString& query_text);
  int get_query_result(const QString& query_text) const;
};
} // namespace labelbuddy

#endif
