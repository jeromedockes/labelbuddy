#include <algorithm>
#include <cassert>

#include <QColor>
#include <QAbstractItemModel>
#include <QVariant>

#include "annotations_list_model.h"
#include "user_roles.h"

namespace labelbuddy {

constexpr int AnnotationsListModel::prefixSize_;
constexpr int AnnotationsListModel::annotationSize_;

AnnotationsListModel::AnnotationsListModel(QObject* parent)
    : QAbstractListModel{parent} {}

QVariant AnnotationsListModel::data(const QModelIndex& index, int role) const {
  if (index.row() > annotations_.size()){
    return QVariant{};
  }
  switch (role) {
  case Qt::BackgroundRole: {
    auto color = labels_[annotations_[index.row()].labelId].color;
    return QColor{color};
  }
  case Roles::AnnotationIdRole: {
    return annotations_[index.row()].id;
  }
  case Roles::LabelNameRole: {
    return labels_[annotations_[index.row()].labelId].name;
  }
  case Roles::SelectedTextRole: {
    auto start = annotations_[index.row()].startChar;
    auto end = annotations_[index.row()].endChar;
    auto selectionSize = std::min(end - start, annotationSize_ - prefixSize_);
    auto result = text_.mid(start, selectionSize);
    if (!result.isValidUtf16()) {
      result = text_.mid(start, selectionSize + 1);
    }
    return result;
  }
  case Roles::AnnotationPrefixRole: {
    auto prefixEnd = annotations_[index.row()].startChar;
    auto prefix = text_.mid(prefixEnd - prefixSize_, prefixSize_);
    if (!prefix.isValidUtf16()) {
      prefix = text_.mid(prefixEnd - (prefixSize_ + 1), (prefixSize_ + 1));
    }
    return prefix;
  }
  case Roles::AnnotationSuffixRole: {
    auto start = annotations_[index.row()].startChar;
    auto end = annotations_[index.row()].endChar;
    auto suffixSize = annotationSize_ - prefixSize_ - (end - start);
    if (suffixSize <= 0) {
      return "";
    }
    auto suffixStart = annotations_[index.row()].endChar;
    auto suffix = text_.mid(suffixStart, suffixSize);
    if (!suffix.isValidUtf16()) {
      suffix = text_.mid(suffixStart, suffixSize + 1);
    }
    return suffix;
  }
  case Roles::AnnotationStartCharRole: {
    return annotations_[index.row()].startChar;
  }
  case Roles::AnnotationExtraDataRole: {
    return annotations_[index.row()].extraData;
  }
  }
  return QVariant{};
}

int AnnotationsListModel::rowCount(const QModelIndex& parent) const {
  (void)parent;
  return annotations_.size();
}

void AnnotationsListModel::setSourceModel(AnnotationsModel* annotationsModel) {
  assert(annotationsModel != nullptr);
  annotationsModel_ = annotationsModel;
  resetAnnotations();
  QObject::connect(annotationsModel_, &AnnotationsModel::documentChanged, this,
                   &AnnotationsListModel::resetAnnotations);
  QObject::connect(annotationsModel_, &AnnotationsModel::annotationAdded, this,
                   &AnnotationsListModel::addAnnotation);
  QObject::connect(annotationsModel_, &AnnotationsModel::annotationDeleted,
                   this, &AnnotationsListModel::deleteAnnotation);
  QObject::connect(annotationsModel_, &AnnotationsModel::extraDataChanged,
                   this, &AnnotationsListModel::updateExtradata);
}

void AnnotationsListModel::addAnnotation(const AnnotationInfo& annotation) {
  beginInsertRows(QModelIndex(), annotations_.size(), annotations_.size());
  annotations_.append(annotation);
  endInsertRows();
}

int AnnotationsListModel::findAnnotationById(int annotationId) const {
  for (int i = 0; i != annotations_.size(); ++i) {
    if (annotations_.at(i).id == annotationId) {
      return i;
    }
  }
  return -1;
}

QModelIndex AnnotationsListModel::indexForAnnotationId(int annotationId) const {
  auto annotationIndex = findAnnotationById(annotationId);
  if (annotationIndex == -1) {
    return QModelIndex{};
  }
  return index(annotationIndex, 0);
}

void AnnotationsListModel::deleteAnnotation(int annotationId) {
  auto annotationIndex = findAnnotationById(annotationId);
  if (annotationIndex == -1) {
    return;
  }
  beginRemoveRows(QModelIndex{}, annotationIndex, annotationIndex);
  annotations_.removeAt(annotationIndex);
  endRemoveRows();
}

void AnnotationsListModel::updateExtradata(int annotationId,
                                           const QString& extraData) {
  auto annotationIndex = findAnnotationById(annotationId);
  if (annotationIndex == -1) {
    return;
  }
  annotations_[annotationIndex].extraData = extraData;
  emit dataChanged(index(annotationIndex, 0), index(annotationIndex, 0),
                   {Roles::AnnotationExtraDataRole});
}

void AnnotationsListModel::resetAnnotations() {
  if (annotationsModel_ == nullptr) {
    assert(false);
    return;
  }
  beginResetModel();
  labels_ = annotationsModel_->getLabelsInfo();
  annotations_ = annotationsModel_->getAnnotationsInfo().values();
  text_ = annotationsModel_->getContent();
  endResetModel();
}

} // namespace labelbuddy
