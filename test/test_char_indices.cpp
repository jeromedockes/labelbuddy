#include <QList>
#include <QMap>
#include <QTest>

#include "char_indices.h"
#include "test_char_indices.h"

namespace labelbuddy {

void TestCharIndices::testUtf8ToQString() {
  CharIndices charIndices{};
  charIndices.setText("aéo𝄞x");
  QMap<int, int> expected{{1, 1}, {3, 2}, {8, 5}};
  auto idx = charIndices.utf8ToQString(expected.keyBegin(), expected.keyEnd());

  QVERIFY(idx == expected);
}

void TestCharIndices::testQStringToUnicode() {
  CharIndices charIndices{};
  charIndices.setText("aéo𝄞x");
  QMap<int, int> expected{{0, 0}, {1, 1}, {2, 2}, {3, 3}, {5, 4}, {6, 5}};
  auto idx =
      charIndices.qStringToUnicode(expected.keyBegin(), expected.keyEnd());

  QVERIFY(idx == expected);
}

void TestCharIndices::testUtf8ToUnicode() {
  CharIndices charIndices{};
  charIndices.setText("aéo𝄞x");
  QMap<int, int> expected{{1, 1}, {3, 2}, {8, 4}};
  auto idx = charIndices.utf8ToUnicode(expected.keyBegin(), expected.keyEnd());

  QVERIFY(idx == expected);

  QMap<int, int> expected_ignore_invalid = {{1, 1}};
  QList<int> input_with_invalid_indices{-2, -1, 1, 12};
  auto idx_ignore_invalid = charIndices.unicodeToUtf8(
      input_with_invalid_indices.begin(), input_with_invalid_indices.end());
  QVERIFY(idx_ignore_invalid == expected_ignore_invalid);
}

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
  QCOMPARE(charIndices.utf8Length(), 0);

  charIndices.setText("aéo𝄞x");
  QCOMPARE(charIndices.unicodeLength(), 5);
  QCOMPARE(charIndices.qStringLength(), 6);
  QCOMPARE(charIndices.utf8Length(), 9);
}

void TestCharIndices::testIsValid() {

  CharIndices charIndices{"aéo𝄞x"};
  QCOMPARE(charIndices.isValidUnicodeIndex(0), true);
  QCOMPARE(charIndices.isValidUnicodeIndex(5), true);
  QCOMPARE(charIndices.isValidUnicodeIndex(6), false);

  QCOMPARE(charIndices.isValidQStringIndex(0), true);
  QCOMPARE(charIndices.isValidQStringIndex(6), true);
  QCOMPARE(charIndices.isValidQStringIndex(7), false);

  QCOMPARE(charIndices.isValidUtf8Index(1), true);
  QCOMPARE(charIndices.isValidUtf8Index(3), true);
  QCOMPARE(charIndices.isValidUtf8Index(4), true);
  QCOMPARE(charIndices.isValidUtf8Index(5), false);
  QCOMPARE(charIndices.isValidUtf8Index(6), false);
  QCOMPARE(charIndices.isValidUtf8Index(7), false);
  QCOMPARE(charIndices.isValidUtf8Index(8), true);
  QCOMPARE(charIndices.isValidUtf8Index(9), true);
  QCOMPARE(charIndices.isValidUtf8Index(10), false);
}

} // namespace labelbuddy
