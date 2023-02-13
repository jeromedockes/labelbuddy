#include <algorithm>
#include <cassert>

#include <QAbstractItemModel>
#include <QColor>
#include <QVariant>

#include "annotations_list_model.h"
#include "user_roles.h"

namespace labelbuddy {

constexpr int AnnotationsListModel::prefixSize_;
constexpr int AnnotationsListModel::annotationSize_;

AnnotationsListModel::AnnotationsListModel(QObject* parent)
    : QAbstractListModel{parent} {}

AnnotationsListModel::AnnotationBoundaries
AnnotationsListModel::getBoundaries(int annotationIndex) const {
  auto anno = annotations_[annotationIndex];
  auto prefixEnd = anno.startChar;
  auto prefixStart = std::max(0, prefixEnd - prefixSize_);
  assert(!text_.isEmpty());
  if (text_.at(prefixStart).isLowSurrogate()) {
    --prefixStart;
    assert(prefixStart >= 0);
    assert(text_.at(prefixStart).isHighSurrogate());
  }
  auto truePrefixSize = prefixEnd - prefixStart;
  auto selectionStart = anno.startChar;
  auto selectionEnd = std::min(
      anno.endChar, selectionStart + (annotationSize_ - truePrefixSize));
  if (selectionEnd != text_.size() && text_.at(selectionEnd).isLowSurrogate()) {
    assert(selectionEnd != anno.endChar);
    ++selectionEnd;
  }
  auto selectionSize = selectionEnd - selectionStart;
  auto suffixStart = anno.endChar;
  auto suffixEnd = anno.endChar;
  if (selectionEnd == anno.endChar) {
    suffixEnd =
        std::max(suffixStart,
                 std::min(text_.size(), suffixStart + annotationSize_ -
                                            truePrefixSize - selectionSize));
    if (suffixEnd != text_.size() && text_.at(suffixEnd).isLowSurrogate()) {
      ++suffixEnd;
    }
  }
  return {prefixStart,  prefixEnd,   selectionStart,
          selectionEnd, suffixStart, suffixEnd};
}

QVariant AnnotationsListModel::data(const QModelIndex& index, int role) const {
  if (index.row() < 0 || index.row() >= annotations_.size()) {
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
  case Roles::AnnotationPrefixRole: {
    auto boundaries = getBoundaries(index.row());
    return text_.mid(boundaries.prefixStart,
                     boundaries.prefixEnd - boundaries.prefixStart);
  }
  case Roles::SelectedTextRole: {
    auto boundaries = getBoundaries(index.row());
    return text_.mid(boundaries.selectionStart,
                     boundaries.selectionEnd - boundaries.selectionStart);
  }
  case Roles::AnnotationSuffixRole: {
    auto boundaries = getBoundaries(index.row());
    return text_.mid(boundaries.suffixStart,
                     boundaries.suffixEnd - boundaries.suffixStart);
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
  QObject::connect(annotationsModel_, &AnnotationsModel::extraDataChanged, this,
                   &AnnotationsListModel::updateExtradata);
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
