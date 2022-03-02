#ifndef LABELBUDDY_SEARCHABLE_TEXT_H
#define LABELBUDDY_SEARCHABLE_TEXT_H

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QWidget>

#include "utils.h"

/// \file
/// read-only plain text display with a search bar and some custom key bindings.

namespace labelbuddy {

/// read-only plain text display with a search bar and some custom key bindings.
class SearchableText : public QWidget {

  Q_OBJECT

public:
  SearchableText(QWidget* parent = nullptr);

  void fill(const QString& content);

  /// start and end character positions for the currently selected text.

  /// returns `{start, end}`
  QList<int> current_selection() const;

  /// The QPlainTextEdit's textCursor
  QTextCursor textCursor() const;

  /// Get a pointer to the child QPlainTextEdit
  QPlainTextEdit* get_text_edit();

  /// Get a pointer to the child search line
  QLineEdit* get_search_box();

public slots:

  void search(QTextDocument::FindFlags flags = QTextDocument::FindFlags());
  void search_forward();
  void search_backward();

protected:
  bool eventFilter(QObject* object, QEvent* event) override;
  void keyPressEvent(QKeyEvent*) override;

private slots:
  /// set position from which next search will start to the current cursor
  /// position
  void set_cursor_position();

  /// enable next/prev buttons iff search box is not empty
  void update_search_button_states();

private:
  QPlainTextEdit* text_edit_;
  QLineEdit* search_box_;
  QPushButton* find_prev_button_;
  QPushButton* find_next_button_;

  QTextCursor last_match_;
  QTextDocument::FindFlags current_search_flags_;

  /// swap the cursor's position and anchor
  void swap_pos_anchor(QTextCursor& cursor) const;
  void handle_nav_event(QKeyEvent* event);

  enum class SelectionSide { Left, Right, Cursor };
  enum class TopOrBottom { Top, Bottom };

  /// scroll until cursor reaches the target position (approximately)

  /// Done by triggering a single step on the scrollbar repeatedly, and not
  /// using the scrollbar's setValue, because we use a QPlainTextEdit which
  /// scrolls paragraph (newline-terminated string rather than wrapped line) by
  /// paragraph, and does not operate on vertical pixels but on paragraphs.
  /// Thus may not reach exactly the target position: it may stop within one
  /// line height of it. (or further if reached end or beginning of text)
  ///
  /// returns true if moved
  bool scroll_to_position(int target, TopOrBottom cursor_side);
  int get_cursor_pos(TopOrBottom top_bottom);

  /// cycle the cursor position between top, center, bottom by scrolling the
  /// text.
  void cycle_cursor_height();

  /// cycle the cursor position between top, center, bottom by scrolling the
  /// text.

  /// Called up to 3 times by `cycle_cursor_position` in case one of the
  /// positions results in no movement (when text cannot be scrolled in one or
  /// both directions).
  void cycle_cursor_height_once();

  /// Key events filtered (handled by this object and not the search bar) when
  /// Ctrl is pressed
  const QList<int> nav_keys_{Qt::Key_K, Qt::Key_J, Qt::Key_N,
                            Qt::Key_P, Qt::Key_U, Qt::Key_D};

  /// Filtered both for the search bar and the text edit
  const QList<QKeySequence::StandardKey> selection_sequences_{
      QKeySequence::SelectNextChar, QKeySequence::SelectPreviousChar,
      QKeySequence::SelectNextWord, QKeySequence::SelectPreviousWord,
      QKeySequence::SelectNextLine, QKeySequence::SelectPreviousLine};

  enum class CursorHeight { Center, Top, Bottom };
  CursorHeight last_cursor_height_{};
  int last_cursor_pos_{};

private slots:

  void extend_selection(QTextCursor::MoveOperation move_op, SelectionSide side);
};
} // namespace labelbuddy

#endif
