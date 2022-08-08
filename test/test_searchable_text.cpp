#include <stdlib.h>

#include "testing_utils.h"

#include "searchable_text.h"
#include "test_searchable_text.h"

namespace labelbuddy {

void TestSearchableText::testSearch() {
  SearchableText text{};
  text.show();
  auto content = exampleDoc();
  text.fill(content);
  QCOMPARE(text.getTextEdit()->toPlainText(), exampleDoc());
  QVERIFY(text.getTextEdit()->isReadOnly());
  auto searchBox = text.findChild<QLineEdit*>();
  auto te = text.findChild<QPlainTextEdit*>();
  searchBox->setText(u8"maçã");

  text.searchForward();
  auto selected =
      content.mid(text.currentSelection()[0],
                  text.currentSelection()[1] - text.currentSelection()[0]);
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

  text.searchBackward();
  cursor = te->textCursor();
  QCOMPARE(cursor.selectedText(), QString(u8"maçã"));
  cursor.clearSelection();
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QCOMPARE(cursor.selectedText(), QString("1"));

  text.searchBackward();
  cursor = te->textCursor();
  QCOMPARE(cursor.selectedText(), QString(u8"maçã"));
  cursor.clearSelection();
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QCOMPARE(cursor.selectedText(), QString("3"));
}

void TestSearchableText::testCyclePos() {
  SearchableText text{};
  text.show();
  text.fill(longDoc());
  auto searchBox = text.findChild<QLineEdit*>();
  auto te = text.findChild<QPlainTextEdit*>();
  searchBox->setText("Line 150");
  text.searchForward();
  QTest::keyClicks(te, "l", Qt::ControlModifier);
  auto cheight = (te->cursorRect().bottom() - te->cursorRect().top());
  QVERIFY((te->cursorRect().bottom() != te->rect().bottom()) &&
          (te->cursorRect().top() != te->rect().top()));
  QTest::keyClicks(te, "l", Qt::ControlModifier);
  QVERIFY(abs(te->cursorRect().top() - te->rect().top()) < cheight);
  QTest::keyClicks(te, "l", Qt::ControlModifier);
  QVERIFY(abs(te->cursorRect().bottom() - te->rect().bottom()) < cheight);
}

void TestSearchableText::testShortcuts() {
  SearchableText text{};
  text.show();
  text.fill(longDoc());
  auto searchBox = text.findChild<QLineEdit*>();
  auto te = text.findChild<QPlainTextEdit*>();
  searchBox->setText("Line 150");
  text.searchForward();

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
