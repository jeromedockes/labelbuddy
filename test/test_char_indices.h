#ifndef LABELBUDDY_TEST_CHAR_INDICES_H
#define LABELBUDDY_TEST_CHAR_INDICES_H

#include <QObject>

namespace labelbuddy {

class TestCharIndices : public QObject {

  Q_OBJECT

private slots:

  void testUtf8ToQString();
  void testQStringToUnicode();
  void testUtf8ToUnicode();

  void testUnicodeToQString();
  void testQStringToUtf8();
  void testUnicodeToUtf8();

  void testUnicodeToQStringSingle();
  void testQStringToUnicodeSingle();

  void testLengths();
  void testIsValid();
};

} // namespace labelbuddy
#endif
