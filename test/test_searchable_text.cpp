#include <stdlib.h>

#include "testing_utils.h"

#include "searchable_text.h"
#include "test_searchable_text.h"

namespace labelbuddy {

void TestSearchableText::test_search() {
  SearchableText text{};
  text.show();
  auto content = example_doc();
  text.fill(content);
  QCOMPARE(text.get_text_edit()->toPlainText(), example_doc());
  QVERIFY(text.get_text_edit()->isReadOnly());
  auto search_box = text.findChild<QLineEdit*>();
  auto te = text.findChild<QPlainTextEdit*>();
  search_box->setText(u8"maçã");

  text.search_forward();
  auto selected =
      content.mid(text.current_selection()[0],
                  text.current_selection()[1] - text.current_selection()[0]);
  QCOMPARE(selected, QString(u8"maçã"));
  auto cursor = te->textCursor();
  QCOMPARE(cursor.selectedText(), QString(u8"maçã"));
  cursor.clearSelection();
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QCOMPARE(cursor.selectedText(), QString("1"));

  text.search();
  cursor = te->textCursor();
  QCOMPARE(cursor.selectedText(), QString(u8"maçã"));
  cursor.clearSelection();
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QCOMPARE(cursor.selectedText(), QString("2"));

  text.search_backward();
  cursor = te->textCursor();
  QCOMPARE(cursor.selectedText(), QString(u8"maçã"));
  cursor.clearSelection();
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QCOMPARE(cursor.selectedText(), QString("1"));

  text.search_backward();
  cursor = te->textCursor();
  QCOMPARE(cursor.selectedText(), QString(u8"maçã"));
  cursor.clearSelection();
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QCOMPARE(cursor.selectedText(), QString("3"));
}

void TestSearchableText::test_cycle_pos() {
  SearchableText text{};
  text.show();
  text.fill(long_doc());
  auto search_box = text.findChild<QLineEdit*>();
  auto te = text.findChild<QPlainTextEdit*>();
  search_box->setText("Line 150");
  text.search_forward();
  QTest::keyClicks(te, "l", Qt::ControlModifier);
  auto cheight = (te->cursorRect().bottom() - te->cursorRect().top());
  QVERIFY((te->cursorRect().bottom() != te->rect().bottom()) &&
          (te->cursorRect().top() != te->rect().top()));
  QTest::keyClicks(te, "l", Qt::ControlModifier);
  QVERIFY(abs(te->cursorRect().top() - te->rect().top()) < cheight);
  QTest::keyClicks(te, "l", Qt::ControlModifier);
  QVERIFY(abs(te->cursorRect().bottom() - te->rect().bottom()) < cheight);
}

void TestSearchableText::test_shortcuts() {
  SearchableText text{};
  text.show();
  text.fill(long_doc());
  auto search_box = text.findChild<QLineEdit*>();
  auto te = text.findChild<QPlainTextEdit*>();
  search_box->setText("Line 150");
  text.search_forward();

  QTest::keyClicks(te, "]");
  QTest::keyClicks(te, "]");
  QTest::keyClicks(te, "]");
  QCOMPARE(te->textCursor().selectedText(), QString("Line 150\u2029Line 151"));

  QTest::keyClicks(te, "[");
  QTest::keyClicks(te, "[");
  QTest::keyClicks(te, "[");
  QCOMPARE(te->textCursor().selectedText(), QString("Line 150"));

  QTest::keyClicks(te, "{");
  QTest::keyClicks(te, "{");
  QTest::keyClicks(te, "{");
  QCOMPARE(te->textCursor().selectedText(), QString("Line 149\u2029Line 150"));

  QTest::keyClicks(te, "}");
  QTest::keyClicks(te, "}");
  QTest::keyClicks(te, "}");
  QCOMPARE(te->textCursor().selectedText(), QString("Line 150"));

  QTest::keyClicks(te, "]", Qt::ControlModifier);
  QTest::keyClicks(te, "]", Qt::ControlModifier);
  QTest::keyClicks(te, "]", Qt::ControlModifier);
  QCOMPARE(te->textCursor().selectedText(), QString("Line 150\u2029Li"));

  QTest::keyClicks(te, "[", Qt::ControlModifier);
  QTest::keyClicks(te, "[", Qt::ControlModifier);
  QTest::keyClicks(te, "[", Qt::ControlModifier);
  QCOMPARE(te->textCursor().selectedText(), QString("Line 150"));

  QTest::keyClicks(te, "{", Qt::ControlModifier);
  QTest::keyClicks(te, "{", Qt::ControlModifier);
  QTest::keyClicks(te, "{", Qt::ControlModifier);
  QCOMPARE(te->textCursor().selectedText(), QString("49\u2029Line 150"));

  QTest::keyClicks(te, "}", Qt::ControlModifier);
  QTest::keyClicks(te, "}", Qt::ControlModifier);
  QTest::keyClicks(te, "}", Qt::ControlModifier);
  QCOMPARE(te->textCursor().selectedText(), QString("Line 150"));
}

} // namespace labelbuddy
