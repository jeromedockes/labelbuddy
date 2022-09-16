#ifndef LABELBUDDY_TEST_CHAR_INDICES_H
#define LABELBUDDY_TEST_CHAR_INDICES_H

#include <qobject.h>

namespace labelbuddy {

class TestCharIndices : public QObject {

  Q_OBJECT

private slots:
  void testUnicodeToQString();
  void testQStringToUtf8();
  void testUnicodeToUtf8();
  void testUnicodeToQStringSingle();
  void testQStringToUnicodeSingle();
  void testLengths();
};

} // namespace labelbuddy
#endif
