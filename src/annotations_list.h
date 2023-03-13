#ifndef LABELBUDDY_ANNOTATIONS_LIST_H
#define LABELBUDDY_ANNOTATIONS_LIST_H

#include <memory>

#include <QAbstractItemModel>
#include <QFrame>
#include <QItemSelectionModel>
#include <QListView>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QEvent>
#include <QObject>

#include "annotations_list_model.h"
#include "annotations_model.h"

/// \file
/// List of annotations shown on the right of the Annotate tab.

namespace labelbuddy {

class AnnotationDelegate : public QStyledItemDelegate {

public:
  AnnotationDelegate(QObject* parent = nullptr);

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;

  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const override;

private:
  int em_;
};

class PainterRestore {
public:
  PainterRestore(QPainter* painter);
  ~PainterRestore();

private:
  QPainter* painter_;
};

/// List of annotations shown on the right of the Annotate tab.

/// A proxy model is used to sort annotations by start char.
class AnnotationsList : public QFrame {
  Q_OBJECT

public:
  AnnotationsList(QWidget* parent = nullptr);
  void setModel(AnnotationsModel* model);

public slots:

  void selectAnnotation(int annotationId);
  void resetAnnotations();

protected:

  bool eventFilter(QObject* object, QEvent* event) override;

private slots:

  void onSelectionChange(const QItemSelection& selected);

signals:

  void selectedAnnotationIdChanged(int annotationId);
  void clicked();

private:
  QListView* annotationsView_;
  AnnotationsModel* annotationsModel_ = nullptr;
  std::unique_ptr<AnnotationsListModel> annotationsListModel_ = nullptr;
  std::unique_ptr<QSortFilterProxyModel> proxyModel_ = nullptr;
};

} // namespace labelbuddy

#endif
