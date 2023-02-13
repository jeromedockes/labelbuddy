#ifndef LABELBUDDY_ANNOTATIONS_LIST_MODEL_H
#define LABELBUDDY_ANNOTATIONS_LIST_MODEL_H

#include <QAbstractItemModel>
#include <QAbstractListModel>

#include "annotations_model.h"

/// \file
/// List of annotations in a document.

namespace labelbuddy {

/// List of annotations in a document.

/// It is a wrapper around the AnnotationsModel that provides data for the
/// annotations list on the right of the "Annotate" tab: annotations, their
/// selected text, and the surrounding context.
class AnnotationsListModel : public QAbstractListModel {
  Q_OBJECT

public:
  AnnotationsListModel(QObject* parent = nullptr);
  QVariant data(const QModelIndex& index, int role) const override;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  void setSourceModel(AnnotationsModel* annotationsModel);
  QModelIndex indexForAnnotationId(int annotationId) const;

public slots:

  /// Updates cached annotation list upon insertion the source model.
  void addAnnotation(const AnnotationInfo& annotation);

  /// Updates cached annotation list upon deletion from the source model.
  void deleteAnnotation(int annotationId);

  /// Updates the cached annotation list upon modification of the source model.

  /// In the current application with `AnnotationsList` the extra data is not
  /// displayed; nevertheless it is available and up-to-date in the model.
  void updateExtradata(int annotationId, const QString& extraData);

  /// Reset the whole annotation list

  /// Used eg when the source model visits a new document or the labels are
  /// changed.
  void resetAnnotations();

private:
  struct AnnotationBoundaries {
    int prefixStart, prefixEnd, selectionStart, selectionEnd, suffixStart,
        suffixEnd;
  };

  /// Find the position of annotation with given ID in the list.

  /// Takes linear time wrt to the number of annotations.
  int findAnnotationById(int annotationId) const;

  AnnotationBoundaries getBoundaries(int annotationIndex) const;

  AnnotationsModel* annotationsModel_ = nullptr;
  QList<AnnotationInfo> annotations_{};
  QMap<int, LabelInfo> labels_{};
  QString text_{};

  /// Approx. length of prefix context that can be shown before the annotation.
  static constexpr int prefixSize_ = 12;

  /// Approx. length of prefix + selected text + suffix.
  static constexpr int annotationSize_ = 200;
};
} // namespace labelbuddy
#endif
