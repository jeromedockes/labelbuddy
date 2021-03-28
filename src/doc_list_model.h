#ifndef LABELBUDDY_DOC_LIST_MODEL_H
#define LABELBUDDY_DOC_LIST_MODEL_H

#include <QPair>
#include <QProgressDialog>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QWidget>

#include "user_roles.h"

/// \file
/// Interface providing information about documents in the database.

namespace labelbuddy {

/// Interface providing information about documents in the database.
class DocListModel : public QSqlQueryModel {

  Q_OBJECT

public:
  DocListModel(QObject* parent = nullptr);

  enum class DocFilter {
    all,
    labelled,
    unlabelled,
    has_given_label,
    not_has_given_label
  };

  /// Number of documents in the database matching the filter params
  int total_n_docs(DocFilter doc_filter = DocFilter::all,
                   int filter_label_id = -1);

  /// data for a document. `Roles::RowIdRole` can be used to get the doc's `id`
  QVariant data(const QModelIndex& index, int role) const override;

  /// Items in the second (hidden) column that contains id cannot be selected.
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  /// label names in the database sorted by id
  QList<QPair<QString, int>> get_label_names() const;

  /// Delete specified docs, reset query and emit `docs_deleted`
  int delete_docs(const QModelIndexList& indices);

  /// Delete all docs, reset query and emit `docs_deleted`
  int delete_all_docs(QProgressDialog* progress = nullptr);

public slots:

  /// Change database
  void set_database(const QString& new_database_name);

  /// Set current query and reset model.
  void adjust_query(DocFilter doc_filter = DocFilter::all,
                    int filter_label_id = -1, int limit = 100, int offset = 0);

  /// reset model
  void refresh_current_query();

  void document_status_changed(DocumentStatus new_status);
  void document_gained_label(int label_id, int doc_id);
  void document_lost_label(int label_id, int doc_id);

  /// called before showing the view; filtered doc list updates are lazy
  void refresh_current_query_if_outdated();

signals:

  void docs_deleted();
  void labels_changed();
  void database_changed();

private:
  QSqlQuery get_query() const;
  void refresh_n_labelled_docs();
  int total_n_docs_no_filter();

  DocFilter doc_filter = DocFilter::all;
  int filter_label_id_ = -1;
  int offset = 0;
  int limit = 100;
  QString database_name;
  bool result_set_outdated_{};

  int n_labelled_docs_{};
};
} // namespace labelbuddy
#endif // LABELBUDDY_DOC_LIST_MODEL_H
