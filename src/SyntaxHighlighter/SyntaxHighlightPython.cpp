/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/SyntaxHighlighter/SyntaxHighlightPython.h>
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> pythonKeywords = {
    //boolean / none
    "False", "None", "True",
    //logical operators
    "and", "in", "is", "not", "or",
    //control flow
    "break", "continue", "elif", "else", "for", "if", "pass", "return", "while",
    //exception handling
    "assert", "except", "finally", "raise", "try",
    //definitions
    "class", "def", "del", "global", "lambda", "nonlocal",
    //imports
    "as", "from", "import",
    //async
    "async", "await",
    //context manager
    "with", "yield"
};

static const std::unordered_set<std::string> pythonTypes = {
    //built-in types
    "bool", "bytearray", "bytes", "complex", "dict",
    "float", "frozenset", "int", "list", "memoryview",
    "object", "set", "slice", "str", "tuple", "type",
    //type hint primitives (python 3.9+)
    "Any", "ClassVar", "Final", "Literal", "Optional", "Union",
    //type hint collections
    "Callable", "Dict", "FrozenSet", "Iterator", "Generator",
    "List", "NamedTuple", "Set", "Sequence", "Tuple", "Type",
    //type hint special forms
    "Awaitable", "Coroutine", "TypeVar", "TypedDict",
    "Protocol", "ParamSpec", "Concatenate", "TypeAlias",
    //abstract base classes
    "ABC", "abstractmethod"
};

static const std::unordered_set<std::string> pythonStdFuncs = {
    //type constructors
    "bool", "bytearray", "bytes", "complex", "dict",
    "float", "frozenset", "int", "list", "memoryview",
    "object", "set", "slice", "str", "tuple", "type",
    //math / comparison
    "abs", "divmod", "max", "min", "pow", "round", "sum",
    //iteration
    "all", "any", "enumerate", "filter", "iter",
    "map", "next", "range", "reversed", "sorted", "zip",
    //introspection
    "callable", "classmethod", "delattr", "dir", "getattr",
    "globals", "hasattr", "hash", "id", "isinstance",
    "issubclass", "locals", "staticmethod", "super", "vars",
    //i/o
    "input", "open", "print",
    //string / encoding
    "ascii", "bin", "chr", "format", "hex", "oct", "ord", "repr",
    //code execution
    "compile", "eval", "exec",
    //misc
    "breakpoint", "help", "len", "property", "setattr"
};

static const std::unordered_set<std::string> pythonDecorators = {
    //built-in decorators
    "classmethod", "staticmethod", "property",
    "abstractmethod", "override",
    //functools
    "cache", "cached_property", "lru_cache", "total_ordering", "wraps",
    //dataclasses
    "dataclass", "field",
    //typing
    "final", "overload", "runtime_checkable"
};

static const std::unordered_set<std::string> pythonExceptions = {
    //base
    "BaseException", "Exception",
    //arithmetic
    "ArithmeticError", "FloatingPointError", "OverflowError", "ZeroDivisionError",
    //lookup
    "AttributeError", "IndexError", "KeyError", "LookupError", "NameError",
    "UnboundLocalError",
    //type / value
    "TypeError", "ValueError", "UnicodeError",
    "UnicodeDecodeError", "UnicodeEncodeError", "UnicodeTranslateError",
    //i/o
    "BlockingIOError", "BrokenPipeError", "BufferError",
    "ConnectionAbortedError", "ConnectionError", "ConnectionRefusedError",
    "ConnectionResetError", "EOFError", "FileExistsError",
    "FileNotFoundError", "InterruptedError", "IsADirectoryError",
    "NotADirectoryError", "OSError", "PermissionError",
    "ProcessLookupError", "TimeoutError",
    //runtime
    "ImportError", "ModuleNotFoundError", "NotImplementedError",
    "RecursionError", "RuntimeError", "StopAsyncIteration",
    "StopIteration", "SystemError",
    //memory
    "MemoryError", "ReferenceError",
    //syntax
    "IndentationError", "SyntaxError", "TabError",
    //system
    "GeneratorExit", "KeyboardInterrupt", "SystemExit",
    //warnings
    "BytesWarning", "DeprecationWarning", "FutureWarning",
    "ImportWarning", "PendingDeprecationWarning", "ResourceWarning",
    "RuntimeWarning", "SyntaxWarning", "UnicodeWarning",
    "UserWarning", "Warning"
};

static const std::unordered_set<std::string> pythonLiterals = {
    "False", "None", "True"
};

//helpers
static bool IsIdentChar(char c) { return std::isalnum((unsigned char)c) || c == '_'; }
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }

void SyntaxHighlightPython::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle();
    textCtrl->SetLexer(wxSTC_LEX_NULL);

    const wxString wxText = textCtrl->GetValue();
    if (wxText.empty()) return;

    const std::string text = wxText.ToStdString();
    const int len = static_cast<int>(text.size());

    std::string styles(len, STYLE_DEFAULT);

    auto setStyle = [&](int from, int to, char style) {
        for (int i = from; i < to && i < len; ++i)
            styles[i] = style;
    };

    int i = 0;
    while (i < len)
    {
        const char c  = text[i];
        const char c1 = (i + 1 < len) ? text[i + 1] : '\0';

        //line comment
        if (c == '#')
        {
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //triple-quoted strings (must be checked before single-quoted)
        if ((c == '"' || c == '\'') && c1 == c && i + 2 < len && text[i + 2] == c)
        {
            int start = i;
            i += 3; //skip opening triple quotes
            while (i + 2 < len && !(text[i] == c && text[i + 1] == c && text[i + 2] == c)) ++i;
            if (i + 2 < len) i += 3; //skip closing triple quotes only if found
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        //single / double quoted strings
        if (c == '"' || c == '\'')
        {
            int start = i;
            const char delim = c;
            ++i; //skip opening quote
            while (i < len && text[i] != '\n')
            {
                if (text[i] == '\\') { i += 2; continue; } //escape sequence
                if (text[i] == delim) { ++i; break; }       //closing quote
                ++i;
            }
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        //decorator (@name)
        if (c == '@' && IsIdentStart(c1))
        {
            int start = i;
            ++i; //skip '@'
            while (i < len && IsIdentChar(text[i])) ++i;
            setStyle(start, i, STYLE_PREPROCESSOR);
            continue;
        }

        //numeric literals (int, float, hex, binary, complex)
        if (std::isdigit((unsigned char)c) || (c == '.' && std::isdigit((unsigned char)c1)))
        {
            int start = i;
            if (c == '0' && (c1 == 'x' || c1 == 'X')) //hex
            {
                i += 2;
                while (i < len && std::isxdigit((unsigned char)text[i])) ++i;
            }
            else if (c == '0' && (c1 == 'b' || c1 == 'B')) //binary
            {
                i += 2;
                while (i < len && (text[i] == '0' || text[i] == '1')) ++i;
            }
            else
            {
                while (i < len && std::isdigit((unsigned char)text[i])) ++i;
                if (i < len && text[i] == '.')
                {
                    ++i;
                    while (i < len && std::isdigit((unsigned char)text[i])) ++i;
                }
                if (i < len && (text[i] == 'e' || text[i] == 'E'))
                {
                    ++i;
                    if (i < len && (text[i] == '+' || text[i] == '-')) ++i;
                    while (i < len && std::isdigit((unsigned char)text[i])) ++i;
                }
                if (i < len && (text[i] == 'j' || text[i] == 'J')) ++i; //complex suffix
            }
            setStyle(start, i, STYLE_NUMBER);
            continue;
        }

        //import -> STYLE_PREPROCESSOR, path -> STYLE_STRING
        if (c == 'i' && text.compare(i, 6, "import") == 0 && (i + 6 >= len || !IsIdentChar(text[i + 6])))
        {
            setStyle(i, i + 6, STYLE_PREPROCESSOR);
            i += 6;
            while (i < len && (text[i] == ' ' || text[i] == '\t')) ++i; // skip whitespace
            int pathStart = i;
            while (i < len && text[i] != '\n') ++i; // rest of the line (could be multiple imports separated by commas, but we'll style them all as string for simplicity)
            setStyle(pathStart, i, STYLE_STRING);
            continue;
        }
        if (c == 'f' && text.compare(i, 4, "from") == 0 && (i + 4 >= len || !IsIdentChar(text[i + 4])))
        {
            setStyle(i, i + 4, STYLE_PREPROCESSOR);
            i += 4;
            while (i < len && (text[i] == ' ' || text[i] == '\t')) ++i; // skip whitespace
            int pathStart = i;
            while (i < len && text[i] != ' ' && text[i] != '\t' && text[i] != '\n') ++i; // module name
            setStyle(pathStart, i, STYLE_STRING);
            continue;
        }

        //identifiers and keywords
        if (IsIdentStart(c))
        {
            int start = i;
            while (i < len && IsIdentChar(text[i])) ++i;
            const std::string word = text.substr(start, i - start);

            if      (pythonKeywords.count(word))    setStyle(start, i, STYLE_KEYWORD);
            else if (pythonLiterals.count(word))     setStyle(start, i, STYLE_NUMBER);
            else if (pythonTypes.count(word))        setStyle(start, i, STYLE_NAMESPACE);
            else if (pythonStdFuncs.count(word))     setStyle(start, i, STYLE_FUNCTION);
            else if (pythonExceptions.count(word))   setStyle(start, i, STYLE_OPERATOR);
            //else: STYLE_DEFAULT
            continue;
        }

        //multi-char operators (checked before single-char)
        if (i + 1 < len)
        {
            static const std::unordered_set<std::string> multiCharOps = {
                "==", "!=", "<=", ">=", "//", "**", ">>", "<<",
                "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "**=", "//="
            };
            const std::string op2 = text.substr(i, 2);
            if (multiCharOps.count(op2))
            {
                setStyle(i, i + 2, STYLE_OPERATOR);
                i += 2;
                continue;
            }
        }

        //single-char operators
        static const std::unordered_set<char> operators = {
            '+', '-', '*', '/', '%', '=', '<', '>', '!',
            '&', '|', '^', '~', ':', ',', '.', '@'
        };
        if (operators.count(c))
        {
            setStyle(i, i + 1, STYLE_OPERATOR);
        }

        ++i;
    }

    //apply styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}