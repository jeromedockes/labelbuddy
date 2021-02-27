#include <QAbstractSlider>
#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLineEdit>
#include <QList>
#include <QPalette>
#include <QPoint>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "searchable_text.h"

namespace labelbuddy {

SearchableText::SearchableText(QWidget* parent) : QWidget(parent) {
  QVBoxLayout* top_layout = new QVBoxLayout();
  setLayout(top_layout);

  text_edit = new QTextEdit();
  top_layout->addWidget(text_edit);
  auto palette = text_edit->palette();
  palette.setColor(QPalette::Inactive, QPalette::Highlight,
                   palette.color(QPalette::Active, QPalette::Highlight));
  palette.setColor(QPalette::Inactive, QPalette::HighlightedText,
                   palette.color(QPalette::Active, QPalette::HighlightedText));
  text_edit->setPalette(palette);

  QHBoxLayout* search_bar_layout = new QHBoxLayout();
  top_layout->addLayout(search_bar_layout);

  search_box = new QLineEdit();
  search_bar_layout->addWidget(search_box);
  search_box->installEventFilter(this);
  find_prev_button = new QPushButton();
  search_bar_layout->addWidget(find_prev_button);
  find_prev_button->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
  find_next_button = new QPushButton();
  search_bar_layout->addWidget(find_next_button);
  find_next_button->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));

  QAction* search_action = new QAction(this);
  search_action->setShortcuts(
      QList<QKeySequence>{QKeySequence::Find, QKeySequence::FindNext});
  QObject::connect(search_action, &QAction::triggered, this,
                   &SearchableText::search_forward);

  QObject::connect(find_next_button, &QPushButton::clicked, this,
                   &SearchableText::search_forward);
  QObject::connect(find_prev_button, &QPushButton::clicked, this,
                   &SearchableText::search_backward);
  QObject::connect(search_box, &QLineEdit::textChanged, this,
                   &SearchableText::update_search_button_states);

  QObject::connect(text_edit, &QTextEdit::selectionChanged, this,
                   &SearchableText::set_cursor_position);
  update_search_button_states();
}

void SearchableText::fill(const QString& content) {
  text_edit->setPlainText(content);
  text_edit->setProperty("readOnly", true);
  this->setFocus();
}

void SearchableText::update_search_button_states() {
  auto has_pattern = search_box->text() != QString();
  find_next_button->setEnabled(has_pattern);
  find_prev_button->setEnabled(has_pattern);
}

void SearchableText::search_forward() { search(); }
void SearchableText::search_backward() { search(QTextDocument::FindBackward); }

void SearchableText::continue_search() { search(current_search_flags); }

void SearchableText::search(QTextDocument::FindFlags flags) {
  this->setFocus();
  auto pattern = search_box->text();
  if (pattern.isEmpty()) {
    return;
  }
  auto document = text_edit->document();
  current_search_flags = flags;
  auto top_left = text_edit->cursorForPosition(text_edit->rect().topLeft());
  auto bottom_right =
      text_edit->cursorForPosition(text_edit->rect().bottomRight());
  if (last_match < top_left || last_match >= bottom_right) {
    last_match =
        (flags & QTextDocument::FindBackward) ? bottom_right : top_left;
  }
  auto found = document->find(pattern, last_match, flags);
  if (found.isNull()) {
    auto new_cursor = text_edit->textCursor();
    if (flags & QTextDocument::FindBackward) {
      new_cursor.movePosition(QTextCursor::End);
    } else {
      new_cursor.movePosition(QTextCursor::Start);
    }
    found = document->find(pattern, new_cursor, flags);
  }
  if (!found.isNull()) {
    last_match = found;
    text_edit->setTextCursor(last_match);
  }
}

void SearchableText::set_cursor_position() {
  last_match = text_edit->textCursor();
}

void SearchableText::swap_pos_anchor(QTextCursor& cursor) const {
  auto pos = cursor.position();
  auto anchor = cursor.anchor();
  cursor.setPosition(pos);
  cursor.setPosition(anchor, QTextCursor::KeepAnchor);
}

void SearchableText::extend_selection(QTextCursor::MoveOperation move_op,
                                      SearchableText::Side side) {
  auto cursor = text_edit->textCursor();
  bool swapped{};
  auto anchor = cursor.anchor();
  auto pos = cursor.position();
  if ((anchor > pos && side == Side::Right) ||
      (anchor < pos && side == Side::Left)) {
    swap_pos_anchor(cursor);
    swapped = true;
  }
  cursor.movePosition(move_op, QTextCursor::KeepAnchor);
  if (swapped) {
    swap_pos_anchor(cursor);
  }
  text_edit->setTextCursor(cursor);
}

bool SearchableText::eventFilter(QObject* object, QEvent* event) {
  if (object == search_box) {
    if (event->type() == QEvent::KeyPress) {
      auto key_event = static_cast<QKeyEvent*>(event);
      if ((nav_keys_nomodif.contains(key_event->key())) ||
          ((key_event->modifiers() & Qt::ControlModifier) &&
           (nav_keys.contains(key_event->key())))) {
        handle_nav_event(key_event);
        return true;
      }
    }
  }
  return QWidget::eventFilter(object, event);
}

void SearchableText::cycle_cursor_height() {
  auto top = text_edit->cursorRect().top();
  for (int i = 0; i < 3; ++i) {
    cycle_cursor_height_once();
    if (text_edit->cursorRect().top() != top) {
      return;
    }
  }
}

void SearchableText::cycle_cursor_height_once() {
  auto pos = text_edit->textCursor().position();
  auto crect = text_edit->cursorRect();
  auto bottom = text_edit->rect().bottom();
  auto top = text_edit->rect().top();
  auto center = (bottom + top) / 2;
  auto sb = text_edit->verticalScrollBar();

  CursorHeight target_height;
  if (pos != last_cursor_pos) {
    target_height = CursorHeight::Center;
    last_cursor_pos = pos;
  } else {
    target_height = static_cast<CursorHeight>(
        (static_cast<int>(last_cursor_height) + 1) % 3);
  }

  switch (target_height) {
  case CursorHeight::Center:
    sb->setValue(sb->value() + crect.bottom() - center);
    break;
  case CursorHeight::Top:
    sb->setValue(sb->value() + crect.top() - top);
    break;
  case CursorHeight::Bottom:
    sb->setValue(sb->value() + crect.bottom() - bottom);
    break;
  }
  last_cursor_height = target_height;
}

void SearchableText::handle_nav_event(QKeyEvent* event) {
  if (((event->key() == Qt::Key_J) &&
       (event->modifiers() & Qt::ControlModifier)) ||
      ((event->key() == Qt::Key_N) &&
       (event->modifiers() & Qt::ControlModifier)) ||
      (event->key() == Qt::Key_Down)) {
    text_edit->verticalScrollBar()->triggerAction(
        QAbstractSlider::SliderSingleStepAdd);
    return;
  }
  if (((event->key() == Qt::Key_K) &&
       (event->modifiers() & Qt::ControlModifier)) ||
      ((event->key() == Qt::Key_P) &&
       (event->modifiers() & Qt::ControlModifier)) ||
      (event->key() == Qt::Key_Up)) {
    text_edit->verticalScrollBar()->triggerAction(
        QAbstractSlider::SliderSingleStepSub);
    return;
  }
  if ((event->key() == Qt::Key_D) &&
      (event->modifiers() & Qt::ControlModifier)) {
    text_edit->verticalScrollBar()->triggerAction(
        QAbstractSlider::SliderPageStepAdd);
    return;
  }
  if ((event->key() == Qt::Key_U) &&
      (event->modifiers() & Qt::ControlModifier)) {
    text_edit->verticalScrollBar()->triggerAction(
        QAbstractSlider::SliderPageStepSub);
    return;
  }
  if ((event->key() == Qt::Key_L) &&
      (event->modifiers() & Qt::ControlModifier)) {
    cycle_cursor_height();
    return;
  }
  if ((event->key() == Qt::Key_BracketRight) &&
      (event->modifiers() & Qt::ControlModifier)) {
    extend_selection(QTextCursor::NextCharacter, Side::Right);
    return;
  }
  if ((event->key() == Qt::Key_BracketLeft) &&
      (event->modifiers() & Qt::ControlModifier)) {
    extend_selection(QTextCursor::PreviousCharacter, Side::Right);
    return;
  }
  if ((event->key() == Qt::Key_BraceRight) &&
      (event->modifiers() & Qt::ControlModifier)) {
    extend_selection(QTextCursor::NextCharacter, Side::Left);
    return;
  }
  if ((event->key() == Qt::Key_BraceLeft) &&
      (event->modifiers() & Qt::ControlModifier)) {
    extend_selection(QTextCursor::PreviousCharacter, Side::Left);
    return;
  }

  if ((event->key() == Qt::Key_BracketRight)) {
    extend_selection(QTextCursor::NextWord, Side::Right);
    return;
  }
  if ((event->key() == Qt::Key_BracketLeft)) {
    extend_selection(QTextCursor::PreviousWord, Side::Right);
    return;
  }
  if ((event->key() == Qt::Key_BraceRight)) {
    extend_selection(QTextCursor::NextWord, Side::Left);
    return;
  }
  if ((event->key() == Qt::Key_BraceLeft)) {
    extend_selection(QTextCursor::PreviousWord, Side::Left);
    return;
  }
  QWidget::keyPressEvent(event);
}

void SearchableText::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::Find) || event->key() == Qt::Key_Slash) {
    search_box->setFocus();
    search_box->selectAll();
    return;
  }
  if (event->matches(QKeySequence::InsertParagraphSeparator)) {
    search_forward();
    return;
  }
  if (event->matches(QKeySequence::InsertLineSeparator)) {
    search_backward();
    return;
  }
  handle_nav_event(event);
}

QTextCursor SearchableText::textCursor() const {
  return text_edit->textCursor();
}
QTextEdit* SearchableText::get_text_edit() { return text_edit; }
QLineEdit* SearchableText::get_search_box() { return search_box; }

QList<int> SearchableText::current_selection() const {
  QTextCursor cursor = text_edit->textCursor();
  return QList<int>{cursor.selectionStart(), cursor.selectionEnd()};
}
} // namespace labelbuddy
