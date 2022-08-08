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
  QList<int> currentSelection() const;

  /// The QPlainTextEdit's textCursor
  QTextCursor textCursor() const;

  /// Get a pointer to the child QPlainTextEdit
  QPlainTextEdit* getTextEdit();

public slots:

  void search(QTextDocument::FindFlags flags = QTextDocument::FindFlags());
  void searchForward();
  void searchBackward();

protected:
  bool eventFilter(QObject* object, QEvent* event) override;
  void keyPressEvent(QKeyEvent*) override;

private slots:
  /// set position from which next search will start to the current cursor
  /// position
  void setCursorPosition();

  /// enable next/prev buttons iff search box is not empty
  void updateSearchButtonStates();

private:
  QPlainTextEdit* textEdit_;
  QLineEdit* searchBox_;
  QPushButton* findPrevButton_;
  QPushButton* findNextButton_;

  QTextCursor lastMatch_;
  QTextDocument::FindFlags currentSearchFlags_;

  /// swap the cursor's position and anchor
  static void swapPosAnchor(QTextCursor& cursor);

  void handleNavEvent(QKeyEvent* event);

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
  bool scrollToPosition(int target, TopOrBottom cursorSide);
  int getCursorPos(TopOrBottom topBottom);

  /// cycle the cursor position between top, center, bottom by scrolling the
  /// text.
  void cycleCursorHeight();

  /// cycle the cursor position between top, center, bottom by scrolling the
  /// text.

  /// Called up to 3 times by `cycleCursorPosition` in case one of the
  /// positions results in no movement (when text cannot be scrolled in one or
  /// both directions).
  void cycleCursorHeightOnce();

  /// Key events filtered (handled by this object and not the search bar) when
  /// Ctrl is pressed
  const QList<int> navKeys_{Qt::Key_K, Qt::Key_J, Qt::Key_N,
                            Qt::Key_P, Qt::Key_U, Qt::Key_D};

  /// Filtered both for the search bar and the text edit
  const QList<QKeySequence::StandardKey> selectionSequences_{
      QKeySequence::SelectNextChar, QKeySequence::SelectPreviousChar,
      QKeySequence::SelectNextWord, QKeySequence::SelectPreviousWord,
      QKeySequence::SelectNextLine, QKeySequence::SelectPreviousLine};

  enum class CursorHeight { Center, Top, Bottom };
  CursorHeight lastCursorHeight_{};
  int lastCursorPos_{};

private slots:

  void extendSelection(QTextCursor::MoveOperation moveOp, SelectionSide side);
};
} // namespace labelbuddy

#endif
