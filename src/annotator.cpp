#include <cassert>

#include <QColor>
#include <QElapsedTimer>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QSqlQueryModel>
#include <QVBoxLayout>
#include <QWidget>

#include "annotator.h"
#include "label_list.h"
#include "searchable_text.h"
#include "user_roles.h"
#include "utils.h"

namespace labelbuddy {

LabelChoices::LabelChoices(QWidget* parent) : QWidget(parent) {
  auto layout = new QVBoxLayout();
  setLayout(layout);
  instructionLabel_ = new QLabel("Set label for selected text:");
  layout->addWidget(instructionLabel_);
  instructionLabel_->setWordWrap(true);
  this->setStyleSheet("QListView::item {background: transparent;}");
  labelsView_ = new NoDeselectAllView();
  layout->addWidget(labelsView_);
  // labelsView->setSpacing(3);
  labelsView_->setFocusPolicy(Qt::NoFocus);
  labelDelegate_.reset(new LabelDelegate);
  labelsView_->setItemDelegate(labelDelegate_.get());

  extraDataLabel_ = new QLabel("&Extra annotation data:");
  layout->addWidget(extraDataLabel_);
  extraDataLabel_->setWordWrap(true);
  extraDataEdit_ = new QLineEdit();
  layout->addWidget(extraDataEdit_);
  extraDataLabel_->setBuddy(extraDataEdit_);

  deleteButton_ = new QPushButton("Delete annotation");
  layout->addWidget(deleteButton_);

  QObject::connect(deleteButton_, &QPushButton::clicked, this,
                   &LabelChoices::onDeleteButtonClick);
  QObject::connect(extraDataEdit_, &QLineEdit::textEdited, this,
                   &LabelChoices::extraDataEdited);
  QObject::connect(extraDataEdit_, &QLineEdit::returnPressed, this,
                   &LabelChoices::extraDataEditFinished);
}

void LabelChoices::setModel(LabelListModel* newModel) {
  assert(newModel != nullptr);
  labelListModel_ = newModel;
  labelsView_->setModel(newModel);
  QObject::connect(labelsView_->selectionModel(),
                   &QItemSelectionModel::selectionChanged, this,
                   &LabelChoices::onSelectionChange);
}

void LabelChoices::onSelectionChange() { emit selectionChanged(); }
void LabelChoices::onDeleteButtonClick() { emit deleteButtonClicked(); }

QModelIndexList LabelChoices::selectedIndexes() const {
  return labelsView_->selectionModel()->selectedIndexes();
}

int LabelChoices::selectedLabelId() const {
  if (labelListModel_ == nullptr) {
    assert(false);
    return -1;
  }
  QModelIndexList selectedIndexes =
      labelsView_->selectionModel()->selectedIndexes();
  auto selected = findFirstInCol0(selectedIndexes);
  if (selected == selectedIndexes.constEnd()) {
    return -1;
  }
  int labelId = labelListModel_->data(*selected, Roles::RowIdRole).toInt();
  return labelId;
}

void LabelChoices::setSelectedLabelId(int labelId) {
  if (labelListModel_ == nullptr) {
    return;
  }
  if (labelId == -1) {
    labelsView_->clearSelection();
    return;
  }
  auto modelIndex = labelListModel_->labelIdToModelIndex(labelId);
  if (!modelIndex.isValid()) {
    return;
  }
  labelsView_->setCurrentIndex(modelIndex);
}

void LabelChoices::setExtraData(const QString& newData) {
  extraDataEdit_->setText(newData);
  extraDataEdit_->setCursorPosition(0);
}

void LabelChoices::enableDeleteAndEdit() {
  deleteButton_->setEnabled(true);
  extraDataEdit_->setEnabled(true);
  extraDataLabel_->setEnabled(true);
}

void LabelChoices::disableDeleteAndEdit() {
  deleteButton_->setDisabled(true);
  extraDataEdit_->setDisabled(true);
  extraDataLabel_->setDisabled(true);
}

void LabelChoices::enableLabelChoice() { labelsView_->setEnabled(true); }
void LabelChoices::disableLabelChoice() { labelsView_->setDisabled(true); }
bool LabelChoices::isLabelChoiceEnabled() const {
  return labelsView_->isEnabled();
}

bool operator<(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  if (lhs.startChar < rhs.startChar) {
    return true;
  }
  if (lhs.startChar > rhs.startChar) {
    return false;
  }
  return lhs.id < rhs.id;
}

bool operator>(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  return rhs < lhs;
}

bool operator==(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  return (lhs.id == rhs.id) && (lhs.startChar == rhs.startChar);
}

bool operator!=(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  return !(lhs == rhs);
}

bool operator<=(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  return (lhs == rhs) || (lhs < rhs);
}

bool operator>=(const AnnotationIndex& lhs, const AnnotationIndex& rhs) {
  return rhs <= lhs;
}

Annotator::Annotator(QWidget* parent) : QSplitter(parent) {
  labelChoices_ = new LabelChoices();
  addWidget(labelChoices_);
  scaleMargin(*labelChoices_, Side::Right);
  auto textFrame = new QFrame();
  addWidget(textFrame);
  auto textLayout = new QVBoxLayout();
  textFrame->setLayout(textLayout);
  textLayout->setContentsMargins(0, 0, 0, 0);
  navButtons_ = new AnnotationsNavButtons();
  textLayout->addWidget(navButtons_);
  titleLabel_ = new QLabel();
  textLayout->addWidget(titleLabel_);
  titleLabel_->setAlignment(Qt::AlignHCenter);
  titleLabel_->setTextInteractionFlags(Qt::TextBrowserInteraction);
  titleLabel_->setOpenExternalLinks(true);
  titleLabel_->setWordWrap(true);
  text_ = new SearchableText();
  textLayout->addWidget(text_);
  scaleMargin(*text_, Side::Left);
  text_->getTextEdit()->installEventFilter(this);
  text_->getTextEdit()->viewport()->installEventFilter(this);
  defaultFormat_ = text_->getTextEdit()->textCursor().charFormat();
  text_->fill("");
  text_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  QSettings settings("labelbuddy", "labelbuddy");
  if (settings.contains("Annotator/state")) {
    restoreState(settings.value("Annotator/state").toByteArray());
  }

  QObject::connect(labelChoices_, &LabelChoices::selectionChanged, this,
                   &Annotator::setLabelForSelectedRegion);
  QObject::connect(labelChoices_, &LabelChoices::deleteButtonClicked, this,
                   &Annotator::deleteActiveAnnotation);
  QObject::connect(labelChoices_, &LabelChoices::extraDataEdited, this,
                   &Annotator::updateExtraDataForActiveAnnotation);
  QObject::connect(labelChoices_, &LabelChoices::extraDataEditFinished, this,
                   &Annotator::setDefaultFocus);
  QObject::connect(this, &Annotator::activeAnnotationChanged, this,
                   &Annotator::paintAnnotations);
  QObject::connect(this, &Annotator::activeAnnotationChanged, this,
                   &Annotator::updateLabelChoicesButtonStates);
  QObject::connect(this, &Annotator::activeAnnotationChanged, this,
                   &Annotator::currentStatusDisplayChanged);
  QObject::connect(text_->getTextEdit(), &QPlainTextEdit::selectionChanged,
                   this, &Annotator::updateLabelChoicesButtonStates);
  QObject::connect(text_->getTextEdit(), &QPlainTextEdit::cursorPositionChanged,
                   this, &Annotator::activateClusterAtCursorPos);

  setFocusProxy(text_);
}

void Annotator::storeState() {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("Annotator/state", saveState());
}

StatusBarInfo Annotator::currentStatusInfo() const {
  if (!annotationsModel_->isPositionedOnValidDoc()) {
    return {"", "", ""};
  }
  StatusBarInfo statusInfo{};
  if (activeAnnotation_ != -1) {
    bool isFirst = activeAnnotation_ == sortedAnnotations_.cbegin()->id;
    bool isFirstInGroup{};
    for (const auto& cluster : clusters_) {
      if (activeAnnotation_ == cluster.firstAnnotation.id) {
        isFirstInGroup = true;
        break;
      }
    }
    statusInfo.annotationInfo =
        QString("%0%1 %2, %3")
            .arg(isFirstInGroup ? "^" : "")
            .arg(isFirst ? "^" : "")
            .arg(annotationsModel_->qStringIdxToUnicodeIdx(
                annotations_[activeAnnotation_].startChar))
            .arg(annotationsModel_->qStringIdxToUnicodeIdx(
                annotations_[activeAnnotation_].endChar));
    statusInfo.annotationLabel =
        labels_[annotations_[activeAnnotation_].labelId].name;
  }
  statusInfo.docInfo = QString("%0 annotation%1 in current doc")
                           .arg(annotations_.size())
                           .arg(annotations_.size() != 1 ? "s" : "");
  return statusInfo;
}

void Annotator::setFont(const QFont& newFont) {
  text_->getTextEdit()->setFont(newFont);
}

void Annotator::setUseBoldFont(bool useBold) {
  useBoldFont_ = useBold;
  paintAnnotations();
}

void Annotator::setAnnotationsModel(AnnotationsModel* newModel) {
  assert(newModel != nullptr);
  annotationsModel_ = newModel;
  navButtons_->setModel(newModel);

  QObject::connect(annotationsModel_, &AnnotationsModel::documentChanged, this,
                   &Annotator::resetDocument);
  QObject::connect(navButtons_, &AnnotationsNavButtons::visitNext,
                   annotationsModel_, &AnnotationsModel::visitNext);
  QObject::connect(navButtons_, &AnnotationsNavButtons::visitPrev,
                   annotationsModel_, &AnnotationsModel::visitPrev);
  QObject::connect(navButtons_, &AnnotationsNavButtons::visitNextLabelled,
                   annotationsModel_, &AnnotationsModel::visitNextLabelled);
  QObject::connect(navButtons_, &AnnotationsNavButtons::visitPrevLabelled,
                   annotationsModel_, &AnnotationsModel::visitPrevLabelled);
  QObject::connect(navButtons_, &AnnotationsNavButtons::visitNextUnlabelled,
                   annotationsModel_, &AnnotationsModel::visitNextUnlabelled);
  QObject::connect(navButtons_, &AnnotationsNavButtons::visitPrevUnlabelled,
                   annotationsModel_, &AnnotationsModel::visitPrevUnlabelled);

  resetDocument();
}

void Annotator::clearAnnotations() {
  deactivateActiveAnnotation();
  text_->getTextEdit()->setExtraSelections({});
  annotations_.clear();
  sortedAnnotations_.clear();
  clusters_.clear();
}

void Annotator::resetDocument() {
  clearAnnotations();
  text_->fill(annotationsModel_->getContent());
  titleLabel_->setText(annotationsModel_->getTitle());
  updateAnnotations();
  emit currentStatusDisplayChanged();
}

void Annotator::updateAnnotations() {
  fetchLabelsInfo();
  fetchAnnotationsInfo();
  updateLabelChoicesButtonStates();
  paintAnnotations();
}

void Annotator::setLabelListModel(LabelListModel* newModel) {
  assert(newModel != nullptr);
  labelChoices_->setModel(newModel);
}

void Annotator::addAnnotationToClusters(const AnnotationCursor& annotation,
                                        std::list<Cluster>& clusters) {
  Cluster newCluster{{annotation.startChar, annotation.id},
                     {annotation.startChar, annotation.id},
                     annotation.startChar,
                     annotation.endChar};
  QList<std::list<Cluster>::const_iterator> clustersToRemove{};
  for (auto cIt = clusters.cbegin(); cIt != clusters.cend(); ++cIt) {
    if (annotation.startChar < cIt->endChar &&
        cIt->startChar < annotation.endChar) {
      clustersToRemove << cIt;
      newCluster.firstAnnotation =
          std::min(newCluster.firstAnnotation, cIt->firstAnnotation);
      newCluster.lastAnnotation =
          std::max(newCluster.lastAnnotation, cIt->lastAnnotation);
      newCluster.startChar = std::min(newCluster.startChar, cIt->startChar);
      newCluster.endChar = std::max(newCluster.endChar, cIt->endChar);
    }
  }
  for (auto cItToRemove : clustersToRemove) {
    clusters.erase(cItToRemove);
  }
  clusters.push_back(newCluster);
}

void Annotator::removeAnnotationFromClusters(const AnnotationCursor& annotation,
                                             std::list<Cluster>& clusters) {
  // find the annotation's cluster
  auto annotationCluster = clusters.cbegin();
  while (annotationCluster != clusters.cend()) {
    if (annotationCluster->startChar <= annotation.startChar &&
        annotationCluster->endChar >= annotation.endChar) {
      break;
    }
    ++annotationCluster;
  }
  if (annotationCluster == clusters.cend()) {
    assert(false);
    return;
  }
  // remove the annotation's cluster and re-insert the other annotations it
  // contains.
  std::list<Cluster> newClusters{};
  auto anno = sortedAnnotations_.find(annotationCluster->firstAnnotation);
  assert(anno != sortedAnnotations_.end());
  while (*anno != annotationCluster->lastAnnotation) {
    if (anno->id != annotation.id) {
      addAnnotationToClusters(annotations_[anno->id], newClusters);
    }
    ++anno;
    assert(anno != sortedAnnotations_.end());
  }
  if (anno->id != annotation.id) {
    addAnnotationToClusters(annotations_[anno->id], newClusters);
  }
  clusters.erase(annotationCluster);
  clusters.splice(clusters.cend(), newClusters);
}

void Annotator::updateLabelChoicesButtonStates() {
  labelChoices_->setSelectedLabelId(activeAnnotationLabel());
  labelChoices_->setExtraData(activeAnnotationExtraData());
  if (activeAnnotation_ != -1) {
    labelChoices_->enableDeleteAndEdit();
    labelChoices_->enableLabelChoice();
    return;
  }
  labelChoices_->disableDeleteAndEdit();
  auto selection = text_->currentSelection();
  if (selection[0] != selection[1]) {
    labelChoices_->enableLabelChoice();
  } else {
    labelChoices_->disableLabelChoice();
  }
}

std::list<Cluster>::const_iterator Annotator::clusterAtPos(int pos) const {
  for (auto cIt = clusters_.cbegin(); cIt != clusters_.cend(); ++cIt) {
    if (cIt->startChar <= pos && cIt->endChar > pos) {
      return cIt;
    }
  }
  return clusters_.cend();
}

void Annotator::updateNavButtons() { navButtons_->updateButtonStates(); }

void Annotator::resetSkipUpdatingNavButtons() {
  navButtons_->setSkipUpdating(false);
}

int Annotator::activeAnnotationLabel() const {
  if (activeAnnotation_ == -1) {
    return -1;
  }
  return annotations_[activeAnnotation_].labelId;
}

QString Annotator::activeAnnotationExtraData() const {
  if (activeAnnotation_ == -1) {
    return "";
  }
  return annotations_[activeAnnotation_].extraData;
}

void Annotator::deactivateActiveAnnotation() {
  if (!annotations_.contains(activeAnnotation_)) {
    return;
  }
  // we only set a charformat if necessary, to avoid re-wrapping long lines
  // unnecessarily when bold is not used
  if (activeAnnoFormatIsSet_) {
    annotations_[activeAnnotation_].cursor.setCharFormat(defaultFormat_);
    activeAnnoFormatIsSet_ = false;
  }
  activeAnnotation_ = -1;
}

void Annotator::activateClusterAtCursorPos() {
  needUpdateActiveAnno_ = false;
  auto cursor = text_->getTextEdit()->textCursor();
  // only activate if click, not drag
  if (cursor.anchor() != cursor.position()) {
    if (activeAnnotation_ != -1) {
      deactivateActiveAnnotation();
      emit activeAnnotationChanged();
    }
    return;
  }
  int annotationId{-1};
  auto cluster = clusterAtPos(cursor.position());
  if (cluster != clusters_.cend()) {
    if (activeAnnotation_ == -1) {
      annotationId = cluster->firstAnnotation.id;
    } else {
      auto active = annotations_[activeAnnotation_];
      auto activeIndex = AnnotationIndex{active.startChar, active.id};
      if (activeIndex < cluster->firstAnnotation ||
          activeIndex > cluster->lastAnnotation) {
        annotationId = cluster->firstAnnotation.id;
      } else {
        auto it = sortedAnnotations_.find(activeIndex);
        if (it->id == cluster->lastAnnotation.id) {
          annotationId = cluster->firstAnnotation.id;
        } else {
          ++it;
          annotationId = it->id;
        }
      }
    }
  }
  if (activeAnnotation_ == annotationId) {
    return;
  }
  deactivateActiveAnnotation();
  activeAnnotation_ = annotationId;
  emit activeAnnotationChanged();
}

void Annotator::deleteAnnotation(int annotationId) {
  if (annotationId == -1) {
    return;
  }
  if (activeAnnotation_ == annotationId) {
    deactivateActiveAnnotation();
  }
  auto deleted = annotationsModel_->deleteAnnotation(annotationId);
  if (!deleted) {
    assert(false);
    emit activeAnnotationChanged();
    return;
  }
  auto anno = annotations_[annotationId];
  removeAnnotationFromClusters(anno, clusters_);
  annotations_.remove(annotationId);
  sortedAnnotations_.erase({anno.startChar, anno.id});
  emit activeAnnotationChanged();
}

void Annotator::deleteActiveAnnotation() {
  deleteAnnotation(activeAnnotation_);
  setDefaultFocus();
}

void Annotator::setDefaultFocus() { text_->setFocus(); }

void Annotator::updateExtraDataForActiveAnnotation(const QString& newData) {
  if (activeAnnotation_ == -1) {
    assert(false);
    return;
  }
  if (annotationsModel_->updateAnnotationExtraData(activeAnnotation_,
                                                   newData)) {
    annotations_[activeAnnotation_].extraData = newData;
  } else {
    assert(false);
  }
}

void Annotator::setLabelForSelectedRegion() {
  if (activeAnnotation_ == -1) {
    addAnnotation();
    return;
  }
  int labelId = labelChoices_->selectedLabelId();
  auto prevLabel = annotations_[activeAnnotation_].labelId;
  if (labelId == prevLabel) {
    return;
  }
  auto start = annotations_[activeAnnotation_].startChar;
  auto end = annotations_[activeAnnotation_].endChar;
  deactivateActiveAnnotation();
  addAnnotation(labelId, start, end);
}

bool Annotator::addAnnotation() {
  int labelId = labelChoices_->selectedLabelId();
  if (labelId == -1) {
    return false;
  }
  QList<int> selectionBoundaries = text_->currentSelection();
  auto start = selectionBoundaries[0];
  auto end = selectionBoundaries[1];
  if (start == end) {
    assert(false);
    return false;
  }
  return addAnnotation(labelId, start, end);
}

bool Annotator::addAnnotation(int labelId, int startChar, int endChar) {
  auto annotationId =
      annotationsModel_->addAnnotation(labelId, startChar, endChar);
  if (annotationId == -1) {
    auto c = text_->getTextEdit()->textCursor();
    c.clearSelection();
    text_->getTextEdit()->setTextCursor(c);
    activeAnnotation_ = -1;
    emit activeAnnotationChanged();
    return false;
  }
  auto annotationCursor = text_->getTextEdit()->textCursor();
  annotationCursor.setPosition(startChar);
  annotationCursor.setPosition(endChar, QTextCursor::KeepAnchor);
  annotations_[annotationId] = AnnotationCursor{
      annotationId, labelId, startChar, endChar, QString(), annotationCursor};
  addAnnotationToClusters(annotations_[annotationId], clusters_);
  sortedAnnotations_.insert({startChar, annotationId});
  auto newCursor = text_->getTextEdit()->textCursor();
  newCursor.clearSelection();
  text_->getTextEdit()->setTextCursor(newCursor);
  deactivateActiveAnnotation();
  activeAnnotation_ = annotationId;
  emit activeAnnotationChanged();
  return true;
}

void Annotator::fetchLabelsInfo() {
  if (annotationsModel_ == nullptr) {
    assert(false);
    return;
  }
  labels_ = annotationsModel_->getLabelsInfo();
}

void Annotator::fetchAnnotationsInfo() {
  if (annotationsModel_ == nullptr) {
    assert(false);
    return;
  }
  int prevActive{activeAnnotation_};
  clearAnnotations();
  auto annotationPositions = annotationsModel_->getAnnotationsInfo();
  for (auto i = annotationPositions.constBegin();
       i != annotationPositions.constEnd(); ++i) {
    auto start = i.value().startChar;
    auto end = i.value().endChar;
    auto cursor = text_->getTextEdit()->textCursor();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    annotations_[i.value().id] =
        AnnotationCursor{i.value().id, i.value().labelId,   start,
                         end,          i.value().extraData, cursor};
    sortedAnnotations_.insert({start, i.value().id});
    addAnnotationToClusters(annotations_[i.value().id], clusters_);
  }
  if (annotations_.contains(prevActive)) {
    activeAnnotation_ = prevActive;
  }
}

int Annotator::findNextAnnotation(AnnotationIndex pos, bool forward) const {
  if (annotations_.empty()) {
    return -1;
  }
  if (forward) {
    auto i = sortedAnnotations_.lower_bound(pos);
    if (i != sortedAnnotations_.cend()) {
      return i->id;
    }
    return sortedAnnotations_.cbegin()->id;
  }
  auto i = sortedAnnotations_.crbegin();
  while ((i != sortedAnnotations_.crend()) && (*i > pos)) {
    ++i;
  }
  if (i != sortedAnnotations_.crend()) {
    return i->id;
  }
  return (sortedAnnotations_.crbegin())->id;
}

void Annotator::selectNextAnnotation(bool forward) {
  AnnotationIndex pos{};
  if (activeAnnotation_ != -1) {
    int offset = forward ? 1 : -1;
    pos = {annotations_[activeAnnotation_].startChar,
           activeAnnotation_ + offset};
  } else {
    pos = {text_->getTextEdit()->textCursor().position(), 0};
  }
  auto nextAnno = findNextAnnotation(pos, forward);
  if (nextAnno == -1) {
    return;
  }
  auto cursor = text_->getTextEdit()->textCursor();
  cursor.setPosition(annotations_[nextAnno].startChar);
  text_->getTextEdit()->setTextCursor(cursor);
  text_->getTextEdit()->ensureCursorVisible();
  deactivateActiveAnnotation();
  activeAnnotation_ = nextAnno;
  emit activeAnnotationChanged();
}

QTextEdit::ExtraSelection
Annotator::makePaintedRegion(int startChar, int endChar, const QString& color,
                             const QString& textColor, bool underline) {
  auto cursor = text_->getTextEdit()->textCursor();
  cursor.setPosition(startChar);
  cursor.setPosition(endChar, QTextCursor::KeepAnchor);

  QTextCharFormat format(defaultFormat_);
  format.setBackground(QColor(color));
  format.setForeground(QColor(textColor));
  format.setFontUnderline(underline);

  return QTextEdit::ExtraSelection{cursor, format};
}

void Annotator::paintAnnotations() {
  const QString darkGray{"#606060"};
  QList<QTextEdit::ExtraSelection> newSelections{};
  int activeStart{-1};
  int activeEnd{-1};
  if (activeAnnotation_ != -1) {
    activeStart = annotations_[activeAnnotation_].startChar;
    activeEnd = annotations_[activeAnnotation_].endChar;
  }
  for (const auto& cluster : clusters_) {
    auto clusterStart = cluster.startChar;
    auto clusterEnd = cluster.endChar;
    if (cluster.lastAnnotation.id != cluster.firstAnnotation.id) {
      if ((clusterStart < activeStart) && (activeStart < clusterEnd)) {
        newSelections << makePaintedRegion(clusterStart, activeStart, darkGray,
                                           "white");
      }
      if ((clusterStart < activeEnd) && (clusterEnd > activeEnd)) {
        newSelections << makePaintedRegion(activeEnd, clusterEnd, darkGray,
                                           "white");
      }
      if ((activeEnd <= clusterStart) || (activeStart >= clusterEnd)) {
        newSelections << makePaintedRegion(clusterStart, clusterEnd, darkGray,
                                           "white");
      }
    } else if (cluster.firstAnnotation.id != activeAnnotation_) {
      newSelections << makePaintedRegion(
          clusterStart, clusterEnd,
          labels_.value(annotations_.value(cluster.firstAnnotation.id).labelId)
              .color);
    }
  }
  if (activeAnnotation_ != -1) {
    auto anno = annotations_[activeAnnotation_];
    if (useBoldFont_) {
      QTextCharFormat fmt(defaultFormat_);
      fmt.setFontWeight(QFont::Bold);
      anno.cursor.setCharFormat(fmt);
      activeAnnoFormatIsSet_ = true;
    } else if (activeAnnoFormatIsSet_) {
      annotations_[activeAnnotation_].cursor.setCharFormat(defaultFormat_);
      activeAnnoFormatIsSet_ = false;
    }
    newSelections << makePaintedRegion(anno.startChar, anno.endChar,
                                       labels_[anno.labelId].color, "black",
                                       true);
  }
  text_->getTextEdit()->setExtraSelections(newSelections);
}

bool Annotator::eventFilter(QObject* object, QEvent* event) {
  if (object == text_->getTextEdit()) {
    if (event->type() == QEvent::KeyPress) {
      auto keyEvent = static_cast<QKeyEvent*>(event);
      if (keyEvent->key() == Qt::Key_Space) {
        keyPressEvent(keyEvent);
        return true;
      }
    }
  }
  if (event->type() == QEvent::MouseButtonPress) {
    mousePressEvent(static_cast<QMouseEvent*>(event));
    return false;
  }
  if (event->type() == QEvent::MouseButtonRelease) {
    mouseReleaseEvent(static_cast<QMouseEvent*>(event));
    return false;
  }
  return QWidget::eventFilter(object, event);
}

void Annotator::mousePressEvent(QMouseEvent* event) {
  (void)event;
  needUpdateActiveAnno_ = true;
}

void Annotator::mouseReleaseEvent(QMouseEvent* event) {
  (void)event;
  if (needUpdateActiveAnno_) {
    activateClusterAtCursorPos();
  }
}

void Annotator::keyPressEvent(QKeyEvent* event) {
  if (event->text() == ">") {
    annotationsModel_->visitNext();
    return;
  }
  if (event->text() == "<") {
    annotationsModel_->visitPrev();
    return;
  }
  if (event->key() == Qt::Key_Escape) {
    deactivateActiveAnnotation();
    emit activeAnnotationChanged();
  }
  if (event->key() == Qt::Key_Space) {
    bool backward(event->modifiers() & Qt::ShiftModifier);
    selectNextAnnotation(!backward);
    return;
  }
  if (!labelChoices_->isLabelChoiceEnabled()) {
    return;
  }
  auto id = annotationsModel_->shortcutToId(event->text());
  if (id != -1) {
    labelChoices_->setSelectedLabelId(id);
    return;
  }
  if (event->key() == Qt::Key_Backspace) {
    deleteActiveAnnotation();
    return;
  }
}

AnnotationsNavButtons::AnnotationsNavButtons(QWidget* parent)
    : QWidget(parent) {
  auto layout = new QHBoxLayout();
  setLayout(layout);

  auto nextIcon = QIcon::fromTheme("go-next", QIcon(":data/icons/go-next.png"));
  auto prevIcon =
      QIcon::fromTheme("go-previous", QIcon(":data/icons/go-previous.png"));
  prevLabelledButton_ = new QPushButton(prevIcon, "labelled");
  layout->addWidget(prevLabelledButton_);
  prevUnlabelledButton_ = new QPushButton(prevIcon, "unlabelled");
  layout->addWidget(prevUnlabelledButton_);
  prevButton_ = new QPushButton(prevIcon, "");
  layout->addWidget(prevButton_);

  currentDocLabel_ = new QLabel();
  layout->addWidget(currentDocLabel_);
  currentDocLabel_->setAlignment(Qt::AlignCenter);

  nextButton_ = new QPushButton(nextIcon, "");
  layout->addWidget(nextButton_);
  nextUnlabelledButton_ = new QPushButton(nextIcon, "unlabelled");
  layout->addWidget(nextUnlabelledButton_);
  nextLabelledButton_ = new QPushButton(nextIcon, "labelled");
  layout->addWidget(nextLabelledButton_);

  QObject::connect(nextButton_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visitNext);
  QObject::connect(prevButton_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visitPrev);
  QObject::connect(nextLabelledButton_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visitNextLabelled);
  QObject::connect(prevLabelledButton_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visitPrevLabelled);
  QObject::connect(nextUnlabelledButton_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visitNextUnlabelled);
  QObject::connect(prevUnlabelledButton_, &QPushButton::clicked, this,
                   &AnnotationsNavButtons::visitPrevUnlabelled);
}

void AnnotationsNavButtons::setModel(AnnotationsModel* newModel) {
  assert(newModel != nullptr);
  annotationsModel_ = newModel;
  QObject::connect(annotationsModel_, &AnnotationsModel::documentChanged, this,
                   &AnnotationsNavButtons::updateButtonStates);
  updateButtonStates();
}

void AnnotationsNavButtons::updateButtonStates() {
  if (annotationsModel_ == nullptr) {
    assert(false);
    return;
  }
  auto docPosition = annotationsModel_->currentDocPosition();
  auto totalDocs = annotationsModel_->totalNDocs();
  auto newMsg = QString("%0 / %1").arg(docPosition + 1).arg(totalDocs);
  if (totalDocs == 0) {
    newMsg = QString("0 / 0");
  }
  currentDocLabel_->setText(newMsg);
  if (skipUpdatingButtons_) {
    return;
  }
  QElapsedTimer timer{};
  timer.start();
  nextButton_->setEnabled(annotationsModel_->hasNext());
  prevButton_->setEnabled(annotationsModel_->hasPrev());
  nextLabelledButton_->setEnabled(annotationsModel_->hasNextLabelled());
  if (timer.elapsed() > skipUpdateButtonsDurationThresholdMs_) {
    return setSkipUpdating(true);
  }
  prevLabelledButton_->setEnabled(annotationsModel_->hasPrevLabelled());
  if (timer.elapsed() > skipUpdateButtonsDurationThresholdMs_) {
    return setSkipUpdating(true);
  }
  nextUnlabelledButton_->setEnabled(annotationsModel_->hasNextUnlabelled());
  if (timer.elapsed() > skipUpdateButtonsDurationThresholdMs_) {
    return setSkipUpdating(true);
  }
  prevUnlabelledButton_->setEnabled(annotationsModel_->hasPrevUnlabelled());
  if (timer.elapsed() > skipUpdateButtonsDurationThresholdMs_) {
    return setSkipUpdating(true);
  }
}

void AnnotationsNavButtons::setSkipUpdating(bool skip) {
  if (skip) {
    skipUpdatingButtons_ = true;
    nextButton_->setEnabled(true);
    prevButton_->setEnabled(true);
    nextLabelledButton_->setEnabled(true);
    prevLabelledButton_->setEnabled(true);
    nextUnlabelledButton_->setEnabled(true);
    prevUnlabelledButton_->setEnabled(true);
    return;
  }
  if (skipUpdatingButtons_) {
    skipUpdatingButtons_ = false;
  }
}

} // namespace labelbuddy
