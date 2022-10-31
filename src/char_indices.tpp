#include <QMap>
#include <QStringRef>
#include <QTextCodec>

#include "char_indices.h"

namespace labelbuddy {

template <typename InputIterator>
QMap<int, int> CharIndices::unicodeToQString(InputIterator unicodeBegin,
                                             InputIterator unicodeEnd) const {
  QMap<int, int> indexMap{};
  while (unicodeBegin != unicodeEnd) {
    if (isValidUnicodeIndex(*unicodeBegin)) {
      indexMap.insert(*unicodeBegin, -1);
    }
    ++unicodeBegin;
  }
  int offset{0};
  auto surrogatesIt = surrogateIndicesInUnicode_.cbegin();
  for (auto mapIt = indexMap.begin(); mapIt != indexMap.end(); ++mapIt) {
    while (surrogatesIt != surrogateIndicesInUnicode_.cend() &&
           *surrogatesIt < mapIt.key()) {
      ++offset;
      ++surrogatesIt;
    }
    mapIt.value() = mapIt.key() + offset;
  }
  return indexMap;
}

template <typename InputIterator>
QMap<int, int> CharIndices::qStringToUtf8(InputIterator qStringBegin,
                                          InputIterator qStringEnd) const {
  QMap<int, int> indexMap{};
  while (qStringBegin != qStringEnd) {
    if (isValidQStringIndex(*qStringBegin)) {
      indexMap.insert(*qStringBegin, -1);
    }
    ++qStringBegin;
  }
  int subStringStart{0};
  int cumLen{0};
  for (auto mapIt = indexMap.begin(); mapIt != indexMap.end(); ++mapIt) {
    auto subString =
        QStringRef(&text_, subStringStart, mapIt.key() - subStringStart);
    cumLen += subString.toUtf8().size();
    mapIt.value() = cumLen;
    subStringStart = mapIt.key();
  }
  return indexMap;
}

template <typename InputIterator>
QMap<int, int> CharIndices::unicodeToUtf8(InputIterator unicodeBegin,
                                          InputIterator unicodeEnd) const {
  auto uniToQs = unicodeToQString(unicodeBegin, unicodeEnd);
  auto qsToUtf8 = qStringToUtf8(uniToQs.cbegin(), uniToQs.cend());
  QMap<int, int> uniToUtf8{};
  for (auto uniToQsItem = uniToQs.cbegin(); uniToQsItem != uniToQs.cend();
       ++uniToQsItem) {
    uniToUtf8.insert(uniToQsItem.key(), qsToUtf8[uniToQsItem.value()]);
  }
  return uniToUtf8;
}

template <typename InputIterator>
QMap<int, int> CharIndices::utf8ToQString(InputIterator utf8Begin,
                                          InputIterator utf8End) const {
  QMap<int, int> indexMap{};
  auto encodedText = text_.toUtf8();
  while (utf8Begin != utf8End) {
    if (isValidUtf8Index(*utf8Begin, encodedText)) {
      indexMap.insert(*utf8Begin, -1);
    }
    ++utf8Begin;
  }
  int subStringStart{0};
  int cumLen{0};
  // using QString::fromUtf8 stops at the first null byte so we use QTextCodec.
  auto codec = QTextCodec::codecForName("UTF-8");
  for (auto mapIt = indexMap.begin(); mapIt != indexMap.end(); ++mapIt) {
    auto subString =
        encodedText.mid(subStringStart, mapIt.key() - subStringStart);
    cumLen += codec->toUnicode(subString).size();
    mapIt.value() = cumLen;
    subStringStart = mapIt.key();
  }
  return indexMap;
}

template <typename InputIterator>
QMap<int, int> CharIndices::qStringToUnicode(InputIterator qStringBegin,
                                             InputIterator qStringEnd) const {
  QMap<int, int> indexMap{};
  while (qStringBegin != qStringEnd) {
    if (isValidQStringIndex(*qStringBegin)) {
      indexMap.insert(*qStringBegin, -1);
    }
    ++qStringBegin;
  }
  int offset{0};
  auto surrogatesIt = surrogateIndicesInQString_.cbegin();
  for (auto mapIt = indexMap.begin(); mapIt != indexMap.end(); ++mapIt) {
    while (surrogatesIt != surrogateIndicesInQString_.cend() &&
           *surrogatesIt < mapIt.key()) {
      --offset;
      ++surrogatesIt;
    }
    mapIt.value() = mapIt.key() + offset;
  }
  return indexMap;
}

template <typename InputIterator>
QMap<int, int> CharIndices::utf8ToUnicode(InputIterator utf8Begin,
                                          InputIterator utf8End) const {
  auto utf8ToQs = utf8ToQString(utf8Begin, utf8End);
  auto qsToUni = qStringToUnicode(utf8ToQs.cbegin(), utf8ToQs.cend());
  QMap<int, int> utf8ToUni{};
  for (auto utf8ToQSItem = utf8ToQs.cbegin(); utf8ToQSItem != utf8ToQs.cend();
       ++utf8ToQSItem) {
    utf8ToUni.insert(utf8ToQSItem.key(), qsToUni[utf8ToQSItem.value()]);
  }
  return utf8ToUni;
}

} // namespace labelbuddy
