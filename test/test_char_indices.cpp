#include <QList>
#include <QMap>
#include <QTest>

#include "char_indices.h"
#include "test_char_indices.h"

namespace labelbuddy {

void TestCharIndices::testUnicodeToQString() {
  CharIndices charIndices{};
  charIndices.setText("aéo𝄞x");
  QMap<int, int> expected{{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 5}, {5, 6}};
  auto qsIdx =
      charIndices.unicodeToQString(expected.keyBegin(), expected.keyEnd());

  QVERIFY(qsIdx == expected);
}

void TestCharIndices::testQStringToUtf8() {
  CharIndices charIndices{"aéo𝄞x"};
  QMap<int, int> expected{{1, 1}, {2, 3}, {5, 8}};
  auto utf8Idx =
      charIndices.qStringToUtf8(expected.keyBegin(), expected.keyEnd());
  QVERIFY(utf8Idx == expected);
}

void TestCharIndices::testUnicodeToUtf8() {
  CharIndices charIndices{};
  charIndices.setText("aéo𝄞x");
  QMap<int, int> expected{{1, 1}, {2, 3}, {4, 8}};
  auto utf8Idx =
      charIndices.unicodeToUtf8(expected.keyBegin(), expected.keyEnd());

  QVERIFY(utf8Idx == expected);
}

void TestCharIndices::testUnicodeToQStringSingle() {
  CharIndices charIndices{"aéo𝄞x"};

  QMap<int, int> expected{{1, 1}, {2, 2}, {4, 5}};
  for (auto item = expected.cbegin(); item != expected.cend(); ++item) {
    QCOMPARE(charIndices.unicodeToQString(item.key()), item.value());
  }
}

void TestCharIndices::testQStringToUnicodeSingle() {
  CharIndices charIndices{"hello"};
  charIndices.setText("aéo𝄞x");

  QMap<int, int> expected{{1, 1}, {2, 2}, {5, 4}};
  for (auto item = expected.cbegin(); item != expected.cend(); ++item) {
    QCOMPARE(charIndices.qStringToUnicode(item.key()), item.value());
  }
}

void TestCharIndices::testLengths() {
  CharIndices charIndices{};
  QCOMPARE(charIndices.unicodeLength(), 0);
  QCOMPARE(charIndices.qStringLength(), 0);

  charIndices.setText("aéo𝄞x");
  QCOMPARE(charIndices.unicodeLength(), 5);
  QCOMPARE(charIndices.qStringLength(), 6);
}

} // namespace labelbuddy
