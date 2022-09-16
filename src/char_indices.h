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

  int unicodeLength() const;
  int qStringLength() const;

  void setText(const QString& newText);

  /// Convert index in Unicode string to index in QString representation.
  int unicodeToQString(int unicodeIndex) const;

  /// Convert index in QString representation to index in Unicode string.
  int qStringToUnicode(int qStringIndex) const;

  /// Convert indices in Unicode string to indices UTF-8 encoded text.

  /// The input iterators provide a sequence of input indices.
  /// The result is a map of input indices to indices in the target
  /// representation.
  template <typename InputIterator>
  QMap<int, int> unicodeToUtf8(InputIterator unicodeBegin,
                               InputIterator unicodeEnd) const;

  /// Convert indices in Unicode string to indices QString representation.

  /// The input iterators provide a sequence of input indices.
  /// The result is a map of input indices to indices in the target
  /// representation.
  template <typename InputIterator>
  QMap<int, int> unicodeToQString(InputIterator unicodeBegin,
                                  InputIterator unicodeEnd) const;

  /// Convert indices in QString representation to indices UTF-8 encoded text.

  /// The input iterators provide a sequence of input indices.
  /// The result is a map of input indices to indices in the target
  /// representation.
  template <typename InputIterator>
  QMap<int, int> qStringToUtf8(InputIterator qStringBegin,
                               InputIterator qStringEnd) const;

private:
  QString text_{""};
  QList<int> surrogateIndicesInUnicode_{};
  QList<int> surrogateIndicesInQString_{};
  int unicodeLength_{};
  int qStringLength_{};
};

} // namespace labelbuddy

#include "char_indices.tpp"

#endif
