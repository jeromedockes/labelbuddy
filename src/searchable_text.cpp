#include <QAbstractSlider>
#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLineEdit>
#include <QList>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPoint>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>

#include "searchable_text.h"

namespace labelbuddy {

SearchableText::SearchableText(QWidget* parent) : QWidget(parent) {
  auto topLayout = new QVBoxLayout();
  setLayout(topLayout);

  textEdit_ = new QPlainTextEdit();
  topLayout->addWidget(textEdit_);
  textEdit_->installEventFilter(this);
  auto palette = textEdit_->palette();
  palette.setColor(QPalette::Inactive, QPalette::Highlight,
                   palette.color(QPalette::Active, QPalette::Highlight));
  palette.setColor(QPalette::Inactive, QPalette::HighlightedText,
                   palette.color(QPalette::Active, QPalette::HighlightedText));
  textEdit_->setPalette(palette);

  auto searchBarLayout = new QHBoxLayout();
  topLayout->addLayout(searchBarLayout);

  searchBox_ = new QLineEdit();
  searchBarLayout->addWidget(searchBox_);
  searchBox_->installEventFilter(this);
  searchBox_->setPlaceholderText("Search in document ( / or Ctrl+F )");
  findPrevButton_ = new QPushButton();
  searchBarLayout->addWidget(findPrevButton_);
  findNextButton_ = new QPushButton();
  searchBarLayout->addWidget(findNextButton_);
  findPrevButton_->setIcon(QIcon(":data/icons/go-up.png"));
  findPrevButton_->setToolTip("Previous search result");
  findNextButton_->setIcon(QIcon(":data/icons/go-down.png"));
  findNextButton_->setToolTip("Next search result");

  auto searchAction = new QAction(this);
  searchAction->setShortcuts(
      QList<QKeySequence>{QKeySequence::Find, QKeySequence::FindNext});
  QObject::connect(searchAction, &QAction::triggered, this,
                   &SearchableText::searchForward);

  QObject::connect(findNextButton_, &QPushButton::clicked, this,
                   &SearchableText::searchForward);
  QObject::connect(findPrevButton_, &QPushButton::clicked, this,
                   &SearchableText::searchBackward);
  QObject::connect(searchBox_, &QLineEdit::textChanged, this,
                   &SearchableText::updateSearchButtonStates);

  QObject::connect(textEdit_, &QPlainTextEdit::selectionChanged, this,
                   &SearchableText::setCursorPosition);
  updateSearchButtonStates();
}

void SearchableText::fill(const QString& content) {
  textEdit_->setPlainText(content);
  textEdit_->setProperty("readOnly", true);
  this->setFocus();
}

void SearchableText::updateSearchButtonStates() {
  auto hasPattern = searchBox_->text() != QString();
  findNextButton_->setEnabled(hasPattern);
  findPrevButton_->setEnabled(hasPattern);
}

void SearchableText::searchForward() { search(); }
void SearchableText::searchBackward() { search(QTextDocument::FindBackward); }

void SearchableText::search(QTextDocument::FindFlags flags) {
  this->setFocus();
  auto pattern = searchBox_->text();
  if (pattern.isEmpty()) {
    return;
  }
  auto document = textEdit_->document();
  currentSearchFlags_ = flags;
  auto topLeft = textEdit_->cursorForPosition(textEdit_->rect().topLeft());
  auto bottomRight =
      textEdit_->cursorForPosition(textEdit_->rect().bottomRight());
  if (lastMatch_ < topLeft || lastMatch_ >= bottomRight) {
    lastMatch_ = (flags & QTextDocument::FindBackward) ? bottomRight : topLeft;
  }
  auto found = document->find(pattern, lastMatch_, flags);
  if (found.isNull()) {
    auto newCursor = textEdit_->textCursor();
    if (flags & QTextDocument::FindBackward) {
      newCursor.movePosition(QTextCursor::End);
    } else {
      newCursor.movePosition(QTextCursor::Start);
    }
    found = document->find(pattern, newCursor, flags);
  }
  if (!found.isNull()) {
    lastMatch_ = found;
    textEdit_->setTextCursor(lastMatch_);
  }
}

void SearchableText::setCursorPosition() {
  lastMatch_ = textEdit_->textCursor();
}

void SearchableText::swapPosAnchor(QTextCursor& cursor) {
  auto pos = cursor.position();
  auto anchor = cursor.anchor();
  cursor.setPosition(pos);
  cursor.setPosition(anchor, QTextCursor::KeepAnchor);
}

void SearchableText::extendSelection(QTextCursor::MoveOperation moveOp,
                                     SelectionSide side) {
  auto cursor = textEdit_->textCursor();
  bool swapped{};
  auto anchor = cursor.anchor();
  auto pos = cursor.position();
  if (anchor == pos) {
    if (side == SelectionSide::Right &&
        (moveOp == QTextCursor::PreviousWord ||
         moveOp == QTextCursor::PreviousCharacter)) {
      return;
    }
    if (side == SelectionSide::Left && (moveOp == QTextCursor::NextWord ||
                                        moveOp == QTextCursor::NextCharacter)) {
      return;
    }
  }
  if ((anchor > pos && side == SelectionSide::Right) ||
      (anchor < pos && side == SelectionSide::Left)) {
    swapPosAnchor(cursor);
    swapped = true;
  }
  cursor.movePosition(moveOp, QTextCursor::KeepAnchor);
  if (swapped) {
    swapPosAnchor(cursor);
  }
  textEdit_->setTextCursor(cursor);
}

bool SearchableText::eventFilter(QObject* object, QEvent* event) {
  if (event->type() == QEvent::KeyPress) {
    auto keyEvent = static_cast<QKeyEvent*>(event);
    if (object == searchBox_) {
      if (((keyEvent->modifiers() & Qt::ControlModifier) &&
           (navKeys_.contains(keyEvent->key())))) {
        handleNavEvent(keyEvent);
        return true;
      }
    }
    for (auto seq : selectionSequences_) {
      if (keyEvent->matches(seq)) {
        handleNavEvent(keyEvent);
        return true;
      }
    }
  }
  return QWidget::eventFilter(object, event);
}

void SearchableText::cycleCursorHeight() {
  auto top = textEdit_->cursorRect().top();
  for (int i = 0; i < 3; ++i) {
    cycleCursorHeightOnce();
    if (textEdit_->cursorRect().top() != top) {
      return;
    }
  }
}

void SearchableText::cycleCursorHeightOnce() {
  textEdit_->ensureCursorVisible();

  auto pos = textEdit_->textCursor().position();
  auto bottom = textEdit_->rect().bottom();
  auto top = textEdit_->rect().top();
  auto center = (bottom + top) / 2;

  CursorHeight targetHeight;
  if (pos != lastCursorPos_) {
    targetHeight = CursorHeight::Center;
    lastCursorPos_ = pos;
  } else {
    targetHeight = static_cast<CursorHeight>(
        (static_cast<int>(lastCursorHeight_) + 1) % 3);
  }
  switch (targetHeight) {
  case CursorHeight::Center:
    scrollToPosition(center, TopOrBottom::Bottom);
    break;
  case CursorHeight::Top:
    scrollToPosition(top, TopOrBottom::Top);
    break;
  case CursorHeight::Bottom:
    scrollToPosition(bottom, TopOrBottom::Bottom);
    break;
  }
  lastCursorHeight_ = targetHeight;
}

bool SearchableText::scrollToPosition(int target, TopOrBottom cursorSide) {
  auto crect = textEdit_->cursorRect();
  auto lineHeight = (crect.bottom() - crect.top());
  auto pos = getCursorPos(cursorSide);
  auto prevPos = pos;
  auto initialPos = pos;
  auto scrollBar = textEdit_->verticalScrollBar();
  if (pos <= target - lineHeight) {
    // scroll up ie move cursor down on the screen
    do {
      prevPos = pos;
      scrollBar->triggerAction(QAbstractSlider::SliderSingleStepSub);
      pos = getCursorPos(cursorSide);
    } while ((pos != prevPos) && (pos <= target - lineHeight));
    return pos != initialPos;
  }

  if (pos >= target + lineHeight) {
    // scroll down ie move cursor up on the screen
    do {
      prevPos = pos;
      scrollBar->triggerAction(QAbstractSlider::SliderSingleStepAdd);
      pos = getCursorPos(cursorSide);
    } while ((pos != prevPos) && (pos >= target + lineHeight));
    return pos != initialPos;
  }
  return false;
}

int SearchableText::getCursorPos(TopOrBottom topBottom) {
  return topBottom == TopOrBottom::Top ? textEdit_->cursorRect().top()
                                       : textEdit_->cursorRect().bottom();
}

void SearchableText::handleNavEvent(QKeyEvent* event) {
  if (((event->key() == Qt::Key_J) &&
       (event->modifiers() & Qt::ControlModifier)) ||
      ((event->key() == Qt::Key_N) &&
       (event->modifiers() & Qt::ControlModifier)) ||
      (event->matches(QKeySequence::MoveToNextLine))) {
    textEdit_->verticalScrollBar()->triggerAction(
        QAbstractSlider::SliderSingleStepAdd);
    return;
  }
  if (((event->key() == Qt::Key_K) &&
       (event->modifiers() & Qt::ControlModifier)) ||
      ((event->key() == Qt::Key_P) &&
       (event->modifiers() & Qt::ControlModifier)) ||
      (event->matches(QKeySequence::MoveToPreviousLine))) {
    textEdit_->verticalScrollBar()->triggerAction(
        QAbstractSlider::SliderSingleStepSub);
    return;
  }
  if (((event->key() == Qt::Key_D) &&
       (event->modifiers() & Qt::ControlModifier)) ||
      event->matches(QKeySequence::MoveToNextPage)) {
    textEdit_->verticalScrollBar()->triggerAction(
        QAbstractSlider::SliderPageStepAdd);
    return;
  }
  if (((event->key() == Qt::Key_U) &&
       (event->modifiers() & Qt::ControlModifier)) ||
      event->matches(QKeySequence::MoveToPreviousPage)) {
    textEdit_->verticalScrollBar()->triggerAction(
        QAbstractSlider::SliderPageStepSub);
    return;
  }
  if (event->key() == Qt::Key_End ||
      event->matches(QKeySequence::MoveToEndOfDocument)) {
    textEdit_->verticalScrollBar()->triggerAction(
        QAbstractSlider::SliderToMaximum);
    return;
  }
  if (event->key() == Qt::Key_Home ||
      event->matches(QKeySequence::MoveToStartOfDocument)) {
    textEdit_->verticalScrollBar()->triggerAction(
        QAbstractSlider::SliderToMinimum);
    return;
  }
  if ((event->key() == Qt::Key_L) &&
      (event->modifiers() & Qt::ControlModifier)) {
    cycleCursorHeight();
    return;
  }
  if ((event->key() == Qt::Key_BracketRight) &&
      (event->modifiers() & Qt::ControlModifier)) {
    extendSelection(QTextCursor::NextCharacter, SelectionSide::Right);
    return;
  }
  if ((event->key() == Qt::Key_BracketLeft) &&
      (event->modifiers() & Qt::ControlModifier)) {
    extendSelection(QTextCursor::PreviousCharacter, SelectionSide::Right);
    return;
  }
  if ((event->key() == Qt::Key_BraceRight) &&
      (event->modifiers() & Qt::ControlModifier)) {
    extendSelection(QTextCursor::NextCharacter, SelectionSide::Left);
    return;
  }
  if ((event->key() == Qt::Key_BraceLeft) &&
      (event->modifiers() & Qt::ControlModifier)) {
    extendSelection(QTextCursor::PreviousCharacter, SelectionSide::Left);
    return;
  }

  if ((event->key() == Qt::Key_BracketRight)) {
    extendSelection(QTextCursor::NextWord, SelectionSide::Right);
    return;
  }
  if ((event->key() == Qt::Key_BracketLeft)) {
    extendSelection(QTextCursor::PreviousWord, SelectionSide::Right);
    return;
  }
  if ((event->key() == Qt::Key_BraceRight)) {
    extendSelection(QTextCursor::NextWord, SelectionSide::Left);
    return;
  }
  if ((event->key() == Qt::Key_BraceLeft)) {
    extendSelection(QTextCursor::PreviousWord, SelectionSide::Left);
    return;
  }
  if (event->matches(QKeySequence::SelectNextChar)) {
    extendSelection(QTextCursor::NextCharacter, SelectionSide::Cursor);
    return;
  }
  if (event->matches(QKeySequence::SelectPreviousChar)) {
    extendSelection(QTextCursor::PreviousCharacter, SelectionSide::Cursor);
    return;
  }
  if (event->matches(QKeySequence::SelectNextWord)) {
    extendSelection(QTextCursor::NextWord, SelectionSide::Cursor);
    return;
  }
  if (event->matches(QKeySequence::SelectPreviousWord)) {
    extendSelection(QTextCursor::PreviousWord, SelectionSide::Cursor);
    return;
  }
  if (event->matches(QKeySequence::SelectNextLine)) {
    extendSelection(QTextCursor::Down, SelectionSide::Cursor);
    return;
  }
  if (event->matches(QKeySequence::SelectPreviousLine)) {
    extendSelection(QTextCursor::Up, SelectionSide::Cursor);
    return;
  }
  QWidget::keyPressEvent(event);
}

void SearchableText::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::Find) || event->key() == Qt::Key_Slash) {
    searchBox_->setFocus();
    searchBox_->selectAll();
    return;
  }
  if (event->matches(QKeySequence::InsertParagraphSeparator)) {
    searchForward();
    return;
  }
  if (event->matches(QKeySequence::InsertLineSeparator)) {
    searchBackward();
    return;
  }
  handleNavEvent(event);
}

QTextCursor SearchableText::textCursor() const {
  return textEdit_->textCursor();
}
QPlainTextEdit* SearchableText::getTextEdit() { return textEdit_; }

QList<int> SearchableText::currentSelection() const {
  QTextCursor cursor = textEdit_->textCursor();
  return QList<int>{cursor.selectionStart(), cursor.selectionEnd()};
}
} // namespace labelbuddy
