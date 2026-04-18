/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/SyntaxHighlighter/SyntaxHighlighter.h>
#include <weditor/SyntaxHighlighter/Text.h>
#include <weditor/SyntaxHighlighter/SyntaxHighlightCPP.h>
#include <weditor/SyntaxHighlighter/SyntaxHighlightCSharp.h>
#include <weditor/SyntaxHighlighter/SyntaxHighlightC.h>
#include <weditor/SyntaxHighlighter/SyntaxHighlightJava.h>
#include <weditor/SyntaxHighlighter/SyntaxHighlightPython.h>
#include <weditor/SyntaxHighlighter/SyntaxHighlightJavaScript.h>
#include <weditor/SyntaxHighlighter/SyntaxHighlightBash.h>
#include <weditor/SyntaxHighlighter/SyntaxHighlightBatch.h>
#include <weditor/SyntaxHighlighter/SyntaxHighlightAssembly.h>
#include <weditor/SyntaxHighlighter/SyntaxHighlightSQL.h>

SyntaxHighlighter* HighlighterFactory::CreateHighlighter(const wxString& language) {
    if (language == "Text") {
        return new Text();
    }
    else if (language == "C++") {
        return new SyntaxHighlightCPP();
    }
    else if (language == "C#") {
        return new SyntaxHighlightCSharp();
    }
    else if (language == "C") {
        return new SyntaxHighlightC();
    }
    else if (language == "Java") {
        return new SyntaxHighlightJava();
    }
    else if (language == "Python") {
        return new SyntaxHighlightPython();    
    }
    else if (language == "JavaScript") {
        return new SyntaxHighlightJavaScript();
    }
    else if (language == "Bash") {
        return new SyntaxHighlightBash();
    }
    else if (language == "Batch") {
        return new SyntaxHighlightBatch();
    }
    else if (language == "Assembly") {
        return new SyntaxHighlightAssembly();
    }
    else if (language == "SQL Script") {
        return new SyntaxHighlightSQL();
    }
    return new Text();
}

std::vector<wxString> HighlighterFactory::GetAvailableLanguages() {
    return {
        "Text",
        "C++",
        "C#",
        "C",
        "Java",
        "Python",
        "JavaScript",
        "Bash",
        "Batch",
        "Assembly",
        "SQL Script"
    };
}
