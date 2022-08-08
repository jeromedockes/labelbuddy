#include <cassert>

#include <QAbstractItemView>
#include <QApplication>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSize>
#include <QStyle>
#include <QStyleFactory>
#include <QVBoxLayout>

#include "compat.h"
#include "label_list.h"
#include "label_list_model.h"
#include "user_roles.h"
#include "utils.h"

namespace labelbuddy {

LabelDelegate::LabelDelegate(bool withDragHandles, QObject* parent)
    : QStyledItemDelegate{parent}, withDragHandles_{withDragHandles} {
  lineWidth_ = QFontMetrics(QFont()).lineWidth();
  margin_ = 2 * lineWidth_;
}

int LabelDelegate::radioButtonWidth() const {
  QStyleOptionButton opt{};
  return QApplication::style()
             ->sizeFromContents(QStyle::CT_RadioButton, &opt, QSize(0, 0))
             .width() +
         2 * lineWidth_;
}

int LabelDelegate::handleWidth() const {
  return withDragHandles_ ? handleOuterWidthFactor_ * lineWidth_ : 0;
}

QStyleOptionViewItem
LabelDelegate::textStyleOption(const QStyleOptionViewItem& option,
                               QRect rect) const {
  QStyleOptionViewItem newOption(option);
  newOption.showDecorationSelected = false;
  auto textPalette = option.palette;
  textPalette.setColor(QPalette::Text, QColor("black"));
  textPalette.setColor(QPalette::Background, QColor(0, 0, 0, 0));
  textPalette.setColor(QPalette::Highlight, QColor(0, 0, 0, 0));
  textPalette.setColor(QPalette::HighlightedText, QColor("black"));
  textPalette.setColor(QPalette::Disabled, QPalette::Text, QColor("#444444"));
  newOption.palette = textPalette;
  newOption.rect = QRect(
      QPoint(rect.left() + handleWidth() + radioButtonWidth(), rect.top()),
      QPoint(rect.right(), rect.bottom()));
  return newOption;
}

void LabelDelegate::paintRadioButton(QPainter* painter,
                                     const QStyleOptionViewItem& option,
                                     const QColor& labelColor) const {
  QStyleOptionButton buttonOpt{};
  buttonOpt.rect =
      QRect(QPoint(option.rect.left() + handleWidth() + 2 * lineWidth_,
                   option.rect.top()),
            option.rect.bottomRight());
  if (option.state & QStyle::State_Selected) {
    buttonOpt.state |= QStyle::State_On;
  } else {
    buttonOpt.state |= QStyle::State_Off;
  }
  if (option.state & QStyle::State_Enabled) {
    buttonOpt.state |= QStyle::State_Enabled;
  } else {
    buttonOpt.state = QStyle::State_NoChange;
    auto palette = buttonOpt.palette;
    palette.setColor(QPalette::Base, labelColor);
    buttonOpt.palette = palette;
  }
  QApplication::style()->drawControl(QStyle::CE_RadioButton, &buttonOpt,
                                     painter);
}

void LabelDelegate::paintDragHandle(QPainter* painter,
                                    const QStyleOptionViewItem& option,
                                    const QColor& labelColor) const {
  (void)labelColor;
  int lw = lineWidth_;
  if (!withDragHandles_) {
    return;
  }
  QRect rect(QPoint(option.rect.left() + handleMarginFactor_ * lw,
                    option.rect.top() + 2),
             QPoint(option.rect.left() + handleMarginFactor_ * lw +
                        handleInnerWidthFactor_ * lw,
                    option.rect.bottom() - 2));
  for (int i = -5; i < 7; i += 3) {
    painter->fillRect(rect.left(), rect.center().y() + i * lw, rect.width(),
                      2 * lw, QColor(255, 255, 255, 90));
    painter->fillRect(rect.left(), rect.center().y() + i * lw, rect.width(), lw,
                      QColor(0, 0, 0, 60));
  }
}

void LabelDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const {
  auto labelColor = index.data(Qt::BackgroundRole).value<QColor>();
  assert(labelColor.isValid());
  QRect innerRect(option.rect);
  innerRect.adjust(0, margin_, 0, -margin_);
  painter->fillRect(innerRect, labelColor);
  paintRadioButton(painter, option, labelColor);
  auto textOption = textStyleOption(option, innerRect);
  QStyledItemDelegate::paint(painter, textOption, index);
  paintDragHandle(painter, option, labelColor);
}

QSize LabelDelegate::sizeHint(const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
  auto size = QStyledItemDelegate::sizeHint(option, index);
  return QSize(size.width() + radioButtonWidth() + handleWidth(),
               size.height() + 4 * margin_);
}

LabelListButtons::LabelListButtons(QWidget* parent) : QFrame(parent) {
  auto outerLayout = new QVBoxLayout();
  setLayout(outerLayout);
  auto topLayout = new QHBoxLayout();
  outerLayout->addLayout(topLayout);
  selectAllButton_ = new QPushButton("Select all");
  topLayout->addWidget(selectAllButton_);
  deleteButton_ = new QPushButton("Delete");
  topLayout->addWidget(deleteButton_);

  auto addLabelLayout = new QHBoxLayout();
  outerLayout->addLayout(addLabelLayout);
  auto addLabelInstruction = new QLabel("&New label:");
  addLabelLayout->addWidget(addLabelInstruction);
  addLabelEdit_ = new QLineEdit();
  addLabelLayout->addWidget(addLabelEdit_);
  addLabelInstruction->setBuddy(addLabelEdit_);
  addLabelEdit_->installEventFilter(this);

  auto bottomLayout = new QHBoxLayout();
  outerLayout->addLayout(bottomLayout);
  setColorButton_ = new QPushButton("Set color");
  bottomLayout->addWidget(setColorButton_);
  shortcutLabel_ = new QLabel("Shortcut key: ");
  bottomLayout->addWidget(shortcutLabel_);
  shortcutEdit_ = new QLineEdit();
  bottomLayout->addWidget(shortcutEdit_);
  shortcutEdit_->setMaxLength(1);
  shortcutEdit_->setValidator(&validator_);
  shortcutEdit_->setFixedWidth(
      textWidth(shortcutEdit_->fontMetrics(), "[a-z]x"));
  shortcutEdit_->setPlaceholderText("[a-z]");
  bottomLayout->addStretch(1);

  QObject::connect(selectAllButton_, &QPushButton::clicked, this,
                   &LabelListButtons::selectAll);
  QObject::connect(setColorButton_, &QPushButton::clicked, this,
                   &LabelListButtons::setLabelColor);
  QObject::connect(deleteButton_, &QPushButton::clicked, this,
                   &LabelListButtons::deleteSelectedRows);
  QObject::connect(shortcutEdit_, &QLineEdit::textEdited, this,
                   &LabelListButtons::setLabelShortcut);
  QObject::connect(addLabelEdit_, &QLineEdit::returnPressed, this,
                   &LabelListButtons::addLabelEditPressed);
}

void LabelListButtons::updateButtonStates(int nSelected, int total,
                                          const QModelIndex& firstSelected) {
  selectAllButton_->setEnabled(total > 0);
  deleteButton_->setEnabled(nSelected > 0);
  setColorButton_->setEnabled(nSelected == 1);
  auto shortcutEditHadFocus = shortcutEdit_->hasFocus();
  shortcutEdit_->setEnabled(nSelected == 1);
  shortcutLabel_->setEnabled(nSelected == 1);
  if (nSelected == 1 && firstSelected.isValid()) {
    shortcutEdit_->setText(firstSelected.model()
                               ->data(firstSelected, Roles::ShortcutKeyRole)
                               .toString());
    shortcutEdit_->setFocus();
  } else {
    if (shortcutEditHadFocus) {
      setFocus();
    }
    shortcutEdit_->setText("");
  }
  if (nSelected != 0) {
    addLabelEdit_->setText("");
    if (addLabelEdit_->hasFocus()) {
      setFocus();
    }
  }
}

void LabelListButtons::setModel(LabelListModel* newModel) {
  assert(newModel != nullptr);
  validator_.setModel(newModel);
}

void LabelListButtons::setView(QListView* newView) {
  assert(newView != nullptr);
  labelListView_ = newView;
  validator_.setView(newView);
}

bool LabelListButtons::eventFilter(QObject* object, QEvent* event) {
  if (object == addLabelEdit_ && event->type() == QEvent::FocusIn) {
    if (labelListView_ != nullptr) {
      labelListView_->clearSelection();
    }
  }
  return QWidget::eventFilter(object, event);
}

void LabelListButtons::addLabelEditPressed() {
  auto labelName = addLabelEdit_->text().trimmed();
  if (labelName != "") {
    emit addLabel(labelName);
  }
  addLabelEdit_->setText("");
}

LabelList::LabelList(QWidget* parent) : QFrame(parent) {
  auto layout = new QVBoxLayout();
  setLayout(layout);

  buttonsFrame_ = new LabelListButtons();
  layout->addWidget(buttonsFrame_);

  this->setStyleSheet("QListView::item {background: transparent;}");
  labelsView_ = new QListView();
  layout->addWidget(labelsView_);
  labelsView_->setDragEnabled(true);
  labelsView_->setAcceptDrops(true);
  labelsView_->setDropIndicatorShown(true);
  labelsView_->setDragDropMode(QAbstractItemView::InternalMove);
  // labelsView->setSpacing(3);
  labelDelegate_.reset(new LabelDelegate(true));
  labelsView_->setItemDelegate(labelDelegate_.get());
  labelsView_->setFocusPolicy(Qt::NoFocus);
  buttonsFrame_->setView(labelsView_);

  labelsView_->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QObject::connect(buttonsFrame_, &LabelListButtons::selectAll, labelsView_,
                   &QListView::selectAll);
  QObject::connect(buttonsFrame_, &LabelListButtons::deleteSelectedRows, this,
                   &LabelList::deleteSelectedRows);
  QObject::connect(buttonsFrame_, &LabelListButtons::setLabelColor, this,
                   &LabelList::setLabelColor);
  QObject::connect(buttonsFrame_, &LabelListButtons::setLabelShortcut, this,
                   &LabelList::setLabelShortcut);
  QObject::connect(buttonsFrame_, &LabelListButtons::addLabel, this,
                   &LabelList::addLabel);
}

void LabelList::setModel(LabelListModel* newModel) {
  assert(newModel != nullptr);
  labelsView_->setModel(newModel);
  buttonsFrame_->setModel(newModel);
  model_ = newModel;
  QObject::connect(model_, &LabelListModel::modelReset, this,
                   &LabelList::updateButtonStates);
  QObject::connect(labelsView_->selectionModel(),
                   &QItemSelectionModel::selectionChanged, this,
                   &LabelList::updateButtonStates);
  updateButtonStates();
}

void LabelList::deleteSelectedRows() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  QModelIndexList selected = labelsView_->selectionModel()->selectedIndexes();
  if (selected.length() == 0) {
    return;
  }
  int resp =
      QMessageBox::question(this, "labelbuddy",
                            QString("Really delete selected labels?\n"
                                    "All associated annotations will be lost"),
                            QMessageBox::Ok | QMessageBox::Cancel);
  if (resp != QMessageBox::Ok) {
    return;
  }
  int nBefore = model_->totalNLabels();
  setFocus();
  model_->deleteLabels(selected);
  int nAfter = model_->totalNLabels();
  int nDeleted = nBefore - nAfter;
  labelsView_->reset();
  QMessageBox::information(
      this, "labelbuddy",
      QString("Deleted %0 label%1").arg(nDeleted).arg(nDeleted > 1 ? "s" : ""),
      QMessageBox::Ok);
}

void LabelList::updateButtonStates() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  auto selected = labelsView_->selectionModel()->selectedIndexes();
  int nRows{};
  for (const auto& sel : selected) {
    if (sel.column() == 0) {
      ++nRows;
    } else {
      assert(false);
    }
  }
  auto firstSelected = findFirstInCol0(selected);
  QModelIndex selectedIndex{};
  if (firstSelected != selected.constEnd()) {
    selectedIndex = *firstSelected;
  }
  buttonsFrame_->updateButtonStates(nRows, model_->totalNLabels(),
                                    selectedIndex);
}

void LabelList::setLabelColor() {
  auto allSelected = labelsView_->selectionModel()->selectedIndexes();
  auto selected = findFirstInCol0(allSelected);
  if (selected == allSelected.constEnd()) {
    return;
  }
  auto labelName = model_->data(*selected, LabelNameRole).toString();
  auto currentColor =
      model_->data(*selected, Qt::BackgroundRole).value<QColor>();
  assert(currentColor.isValid());
  auto color = QColorDialog::getColor(
      currentColor, this, QString("Set color for '%0'").arg(labelName));
  model_->setLabelColor(*selected, color);
}

void LabelList::setLabelShortcut(const QString& newShortcut) {
  auto allSelected = labelsView_->selectionModel()->selectedIndexes();
  auto selected = findFirstInCol0(allSelected);
  if (selected == allSelected.constEnd()) {
    return;
  }
  model_->setLabelShortcut(*selected, newShortcut);
}

void LabelList::addLabel(const QString& name) {
  auto labelId = model_->addLabel(name);
  if (labelId == -1) {
    return;
  }
  labelsView_->reset();
  auto modelIndex = model_->labelIdToModelIndex(labelId);
  assert(modelIndex.isValid());
  labelsView_->setCurrentIndex(modelIndex);
}

QValidator::State ShortcutValidator::validate(QString& input, int& pos) const {
  (void)pos;
  if (model_ == nullptr || view_ == nullptr) {
    return State::Invalid;
  }
  if (input == QString("")) {
    return State::Acceptable;
  }
  auto allSelected = view_->selectionModel()->selectedIndexes();
  QModelIndex selected{};
  auto selectedIt = findFirstInCol0(allSelected);
  if (selectedIt != allSelected.constEnd()) {
    selected = *selectedIt;
  }
  if (!model_->isValidShortcut(input, selected)) {
    return State::Invalid;
  }
  return State::Acceptable;
}

void ShortcutValidator::setModel(LabelListModel* newModel) {
  assert(newModel != nullptr);
  model_ = newModel;
}

void ShortcutValidator::setView(QListView* newView) {
  assert(newView != nullptr);
  view_ = newView;
}

} // namespace labelbuddy
