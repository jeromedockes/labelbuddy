#include <QMap>
#include <QStringRef>
#include <QTextCodec>

namespace labelbuddy {

template <typename InputIterator>
QMap<int, int> CharIndices::unicodeToQString(InputIterator unicodeBegin,
                                             InputIterator unicodeEnd) const {
  QMap<int, int> indexMap{};
  while (unicodeBegin != unicodeEnd) {
    assert(0 <= *unicodeBegin && *unicodeBegin <= unicodeLength_);

    indexMap.insert(*unicodeBegin, -1);
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
    assert(0 <= *qStringBegin && *qStringBegin <= qStringLength_);

    indexMap.insert(*qStringBegin, -1);
    ++qStringBegin;
  }
  int substringStart{0};
  int cumLen{0};
  for (auto mapIt = indexMap.begin(); mapIt != indexMap.end(); ++mapIt) {
    auto subString =
        QStringRef(&text_, substringStart, mapIt.key() - substringStart);
    cumLen += subString.toUtf8().size();
    mapIt.value() = cumLen;
    substringStart = mapIt.key();
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

} // namespace labelbuddy
