#ifndef LABELBUDDY_CHAR_INDICES_H
#define LABELBUDDY_CHAR_INDICES_H

#include <QMap>
#include <QString>

/// \file
/// Conversions of annotation positions between QString, Unicode string and
/// UTF-8 encoded document text

namespace labelbuddy {

/// Handle conversions between character positions in QString, Unicode and UTF-8

/// QStrings are UTF-16 characters: most unicode characters (code points) count
/// as 1, and some as 2 characters (surrogate pairs). QTextEdit, the cursor etc.
/// work with these indices. In the database and upon export we report
/// annotation positions in terms of Unicode characters -- ie one code point
/// counts as 1 character. That is also how sqlite counts indices, lengths etc
/// for the text data type. Upon export we also add the positions as indices
/// into the UTF-8 encoded text.
///
/// This class handles the conversions between these different ways of counting
/// characters.
class CharIndices {
public:
  CharIndices() = default;

  explicit CharIndices(const QString& text);

  /// Length of text as string of Unicode chars.
  int unicodeLength() const;

  /// Length of text as a QString.
  int qStringLength() const;

  /// Length (in bytes) of UTF-8 encoded text.

  /// More expensive than other lengths because it requires encoding the text.
  int utf8Length() const;

  void setText(const QString& newText);

  /// Convert index in Unicode string to index in QString representation.
  int unicodeToQString(int unicodeIndex) const;

  /// Convert index in QString representation to index in Unicode string.
  int qStringToUnicode(int qStringIndex) const;

  /// Convert indices in Unicode string to indices in UTF-8 text.

  /// The input iterators provide a sequence of input indices.
  /// The result is a map of input indices to indices in the target
  /// representation.
  /// Invalid indices (eg < 0) will be missing from the resulting map.
  template <typename InputIterator>
  QMap<int, int> unicodeToUtf8(InputIterator unicodeBegin,
                               InputIterator unicodeEnd) const;

  /// Convert indices in Unicode string to indices QString representation.

  /// The input iterators provide a sequence of input indices.
  /// The result is a map of input indices to indices in the target
  /// representation.
  /// Invalid indices (eg < 0) will be missing from the resulting map.
  template <typename InputIterator>
  QMap<int, int> unicodeToQString(InputIterator unicodeBegin,
                                  InputIterator unicodeEnd) const;

  /// Convert indices in QString representation to indices in UTF-8 text.

  /// The input iterators provide a sequence of input indices.
  /// The result is a map of input indices to indices in the target
  /// representation.
  /// Invalid indices (eg < 0) will be missing from the resulting map.
  template <typename InputIterator>
  QMap<int, int> qStringToUtf8(InputIterator qStringBegin,
                               InputIterator qStringEnd) const;

  /// Convert indices in UTF-8 text to indices in Unicode string.

  /// The input iterators provide a sequence of input indices.
  /// The result is a map of input indices to indices in the target
  /// representation.
  /// Invalid indices (eg < 0) will be missing from the resulting map.
  template <typename InputIterator>
  QMap<int, int> utf8ToUnicode(InputIterator utf8Begin,
                               InputIterator utf8End) const;

  /// Convert indices in UTF-8 text to indices in QString representation.

  /// The input iterators provide a sequence of input indices.
  /// The result is a map of input indices to indices in the target
  /// representation.
  /// Invalid indices (eg < 0) will be missing from the resulting map.
  template <typename InputIterator>
  QMap<int, int> utf8ToQString(InputIterator utf8Begin,
                               InputIterator utf8End) const;

  /// Convert indices in QString representation to indices in Unicode string.

  /// The input iterators provide a sequence of input indices.
  /// The result is a map of input indices to indices in the target
  /// representation.
  /// Invalid indices (eg < 0) will be missing from the resulting map.
  template <typename InputIterator>
  QMap<int, int> qStringToUnicode(InputIterator qStringBegin,
                                  InputIterator qStringEnd) const;

  /// Whether a Unicode char position is valid for this text.
  bool isValidUnicodeIndex(int index) const;

  /// Whether a QChar position is valid for this text.
  bool isValidQStringIndex(int index) const;

  /// Whether a UTF8 byte position is valid for this text.

  /// Checks if it is within range and not in the middle of a multi-byte
  /// sequence. This operation is somewhat expensive because it requires
  /// encoding the text (the UTF8-encoded text is not stored).
  bool isValidUtf8Index(int index) const;

private:
  QString text_{""};
  QList<int> surrogateIndicesInUnicode_{};
  QList<int> surrogateIndicesInQString_{};
  int unicodeLength_{};
  int qStringLength_{};

  static bool isValidUtf8Index(int index, const QByteArray& text);
};

} // namespace labelbuddy

#include "char_indices.tpp"

#endif
