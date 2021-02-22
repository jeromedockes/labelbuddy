#ifndef LABELBUDDY_LABEL_LIST_MODEL_H
#define LABELBUDDY_LABEL_LIST_MODEL_H

#include <QSqlQuery>
#include <QSqlQueryModel>

namespace labelbuddy {

class LabelListModel : public QSqlQueryModel {

  Q_OBJECT

public:
  LabelListModel(QObject* parent = nullptr);

  QVariant data(const QModelIndex& index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  QModelIndex label_id_to_model_index(int label_id) const;

  int total_n_labels() const;

  int delete_labels(const QModelIndexList& indices);

public slots:

  void set_database(const QString& new_database_name);
  void refresh_current_query();
  void set_label_color(const QModelIndex& index, const QString& color);

signals:

  void labels_changed();
  void labels_deleted();

private:
  QSqlQuery get_query() const;

  QString database_name;
};
} // namespace labelbuddy
#endif // LABELBUDDY_DOC_LIST_MODEL_H
