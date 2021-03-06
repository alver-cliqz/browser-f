// |reftest| error:SyntaxError
// Copyright (C) 2016 the V8 project authors. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.
/*---
esid: sec-html-like-comments
es6id: B1.3
description: >
    Arbitrary character sequence not permitted before HTMLCloseComment token
info: |
    Comment ::
      MultiLineComment
      SingleLineComment
      SingleLineHTMLOpenComment
      SingleLineHTMLCloseComment
      SingleLineDelimitedComment

    MultiLineComment ::
      /* FirstCommentLine[opt] LineTerminator MultiLineCommentChars[opt] * / HTMLCloseComment[opt]
negative:
  phase: early
  type: SyntaxError
---*/

throw "Test262: This statement should not be evaluated.";

/*
*/ the comment should not include these characters, regardless of AnnexB extensions -->
