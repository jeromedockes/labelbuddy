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
  find_prev_button = new QPushButton("Prev");
  search_bar_layout->addWidget(find_prev_button);
  find_next_button = new QPushButton("Next");
  search_bar_layout->addWidget(find_next_button);

  QAction* search_action = new QAction(this);
  search_action->setShortcuts(
      QList<QKeySequence>{QKeySequence::Find, QKeySequence::FindNext});
  QObject::connect(search_action, &QAction::triggered, this,
                   &SearchableText::search_forward);

  QObject::connect(find_next_button, &QPushButton::clicked, this,
                   &SearchableText::search_forward);
  QObject::connect(find_prev_button, &QPushButton::clicked, this,
                   &SearchableText::search_backward);
  // QObject::connect(search_box, &QLineEdit::textChanged, this,
  //                  &SearchableText::continue_search);

  QObject::connect(text_edit, &QTextEdit::selectionChanged, this,
                   &SearchableText::set_cursor_position);
}

void SearchableText::fill(const QString& content) {
  text_edit->setText(content);
  text_edit->setProperty("readOnly", true);
  text_edit->setFocus();
}

void SearchableText::search_forward() { search(); }
void SearchableText::search_backward() { search(QTextDocument::FindBackward); }

void SearchableText::continue_search() { search(current_search_flags); }

void SearchableText::search(QTextDocument::FindFlags flags) {
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

bool SearchableText::eventFilter(QObject* object, QEvent* event) {
  if (object == search_box) {
    if (event->type() == QEvent::KeyPress) {
      auto key_event = static_cast<QKeyEvent*>(event);
      if ((key_event->modifiers() & Qt::ControlModifier) &&
          (nav_keys.contains(key_event->key()))) {
        handle_nav_event(key_event);
        return true;
      }
    }
  }
  return QWidget::eventFilter(object, event);
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
  QWidget::keyPressEvent(event);
}

void SearchableText::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::Find)) {
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

QList<int> SearchableText::current_selection() const {
  QTextCursor cursor = text_edit->textCursor();
  return QList<int>{cursor.selectionStart(), cursor.selectionEnd()};
}
} // namespace labelbuddy
