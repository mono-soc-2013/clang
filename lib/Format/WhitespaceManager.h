//===--- WhitespaceManager.h - Format C++ code ------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief WhitespaceManager class manages whitespace around tokens and their
/// replacements.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_FORMAT_WHITESPACEMANAGER_H
#define LLVM_CLANG_FORMAT_WHITESPACEMANAGER_H

#include "TokenAnnotator.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Format/Format.h"
#include <string>

namespace clang {
namespace format {

/// \brief Manages the whitespaces around tokens and their replacements.
///
/// This includes special handling for certain constructs, e.g. the alignment of
/// trailing line comments.
///
/// To guarantee correctness of alignment operations, the \c WhitespaceManager
/// must be informed about every token in the source file; for each token, there
/// must be exactly one call to either \c replaceWhitespace or
/// \c addUntouchableToken.
///
/// There may be multiple calls to \c breakToken for a given token.
class WhitespaceManager {
public:
  WhitespaceManager(SourceManager &SourceMgr, const FormatStyle &Style)
      : SourceMgr(SourceMgr), Style(Style) {}

  /// \brief Replaces the whitespace in front of \p Tok. Only call once for
  /// each \c AnnotatedToken.
  void replaceWhitespace(const AnnotatedToken &Tok, unsigned Newlines,
                         unsigned Spaces, unsigned StartOfTokenColumn,
                         bool InPPDirective = false);

  /// \brief Adds information about an unchangable token's whitespace.
  ///
  /// Needs to be called for every token for which \c replaceWhitespace
  /// was not called.
  void addUntouchableToken(const FormatToken &Tok, bool InPPDirective);

  /// \brief Inserts a line break into the middle of a token.
  ///
  /// Will break at \p Offset inside \p Tok, putting \p PreviousPostfix before
  /// the line break and \p CurrentPrefix before the rest of the token starts in
  /// the next line.
  ///
  /// \p InPPDirective and \p Spaces are used to generate the correct line
  /// break.
  void breakToken(const FormatToken &Tok, unsigned Offset,
                  unsigned ReplaceChars, StringRef PreviousPostfix,
                  StringRef CurrentPrefix, bool InPPDirective, unsigned Spaces);

  /// \brief Returns all the \c Replacements created during formatting.
  const tooling::Replacements &generateReplacements();

private:
  /// \brief Represents a change before a token, a break inside a token,
  /// or the layout of an unchanged token (or whitespace within).
  struct Change {
    /// \brief Functor to sort changes in original source order.
    class IsBeforeInFile {
     public:
      IsBeforeInFile(const SourceManager &SourceMgr) : SourceMgr(SourceMgr) {}
      bool operator()(const Change &C1, const Change &C2) const;

     private:
      const SourceManager &SourceMgr;
    };

    Change() {}

    /// \brief Creates a \c Change.
    ///
    /// The generated \c Change will replace the characters at
    /// \p OriginalWhitespaceRange with a concatenation of
    /// \p PreviousLinePostfix, \p NewlinesBefore line breaks, \p Spaces spaces
    /// and \p CurrentLinePrefix.
    ///
    /// \p StartOfTokenColumn and \p InPPDirective will be used to lay out
    /// trailing comments and escaped newlines.
    Change(bool CreateReplacement, const SourceRange &OriginalWhitespaceRange,
           unsigned Spaces, unsigned StartOfTokenColumn,
           unsigned NewlinesBefore, StringRef PreviousLinePostfix,
           StringRef CurrentLinePrefix, tok::TokenKind Kind,
           bool ContinuesPPDirective);

    bool CreateReplacement;
    // Changes might be in the middle of a token, so we cannot just keep the
    // FormatToken around to query its information.
    SourceRange OriginalWhitespaceRange;
    unsigned StartOfTokenColumn;
    unsigned NewlinesBefore;
    std::string PreviousLinePostfix;
    std::string CurrentLinePrefix;
    // The kind of the token whose whitespace this change replaces, or in which
    // this change inserts whitespace.
    // FIXME: Currently this is not set correctly for breaks inside comments, as
    // the \c BreakableToken is still doing its own alignment.
    tok::TokenKind Kind;
    bool ContinuesPPDirective;

    // The number of spaces in front of the token or broken part of the token.
    // This will be adapted when aligning tokens.
    unsigned Spaces;

    // \c IsTrailingComment, \c TokenLength, \c PreviousEndOfTokenColumn and
    // \c EscapedNewlineColumn will be calculated in
    // \c calculateLineBreakInformation.
    bool IsTrailingComment;
    unsigned TokenLength;
    unsigned PreviousEndOfTokenColumn;
    unsigned EscapedNewlineColumn;

  };

  /// \brief Calculate \c IsTrailingComment, \c TokenLength for the last tokens
  /// or token parts in a line and \c PreviousEndOfTokenColumn and
  /// \c EscapedNewlineColumn for the first tokens or token parts in a line.
  void calculateLineBreakInformation();

  /// \brief Align trailing comments over all \c Changes.
  void alignTrailingComments();

  /// \brief Align trailing comments from change \p Start to change \p End at
  /// the specified \p Column.
  void alignTrailingComments(unsigned Start, unsigned End, unsigned Column);

  /// \brief Align escaped newlines over all \c Changes.
  void alignEscapedNewlines();

  /// \brief Align escaped newlines from change \p Start to change \p End at
  /// the specified \p Column.
  void alignEscapedNewlines(unsigned Start, unsigned End, unsigned Column);

  /// \brief Fill \c Replaces with the replacements for all effective changes.
  void generateChanges();

  /// \brief Stores \p Text as the replacement for the whitespace in \p Range.
  void storeReplacement(const SourceRange &Range, StringRef Text);
  std::string getNewLineText(unsigned NewLines, unsigned Spaces);
  std::string getNewLineText(unsigned NewLines, unsigned Spaces,
                             unsigned PreviousEndOfTokenColumn,
                             unsigned EscapedNewlineColumn);
  std::string getIndentText(unsigned Spaces);

  SmallVector<Change, 16> Changes;
  SourceManager &SourceMgr;
  tooling::Replacements Replaces;
  const FormatStyle &Style;
};

} // namespace format
} // namespace clang

#endif // LLVM_CLANG_FORMAT_WHITESPACEMANAGER_H
