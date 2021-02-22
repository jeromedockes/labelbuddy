#ifndef LABELBUDDY_DOC_LIST_MODEL_H
#define LABELBUDDY_DOC_LIST_MODEL_H

#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QWidget>
#include <QProgressDialog>

namespace labelbuddy {

class DocListModel : public QSqlQueryModel {

  Q_OBJECT

public:
  DocListModel(QObject* parent = nullptr);

  enum DocFilter { all, labelled, unlabelled };

  int total_n_docs(DocFilter doc_filter = DocFilter::all);

  QVariant data(const QModelIndex& index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  int delete_docs(const QModelIndexList& indices);

  int delete_all_docs(QProgressDialog* progress=nullptr);

public slots:

  void set_database(const QString& new_database_name);
  void adjust_query(DocFilter doc_filter = DocFilter::all, int limit = 100,
                    int offset = 0);
  void refresh_current_query();

signals:

  void docs_deleted();

private:
  QSqlQuery get_query() const;

  DocFilter doc_filter = DocFilter::all;
  int offset = 0;
  int limit = 100;
  QString database_name;
};
} // namespace labelbuddy
#endif // LABELBUDDY_DOC_LIST_MODEL_H
