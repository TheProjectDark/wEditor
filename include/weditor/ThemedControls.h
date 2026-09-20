/*
 * wEditor Themed Controls
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <algorithm>
#include <vector>

//Buttons and a dropdown that draw themselves.
//
//Native buttons and combo boxes can't be trusted to follow the colours of the app:
// - macOS draws the bezel of an NSButton itself, so SetBackgroundColour is ignored or looks
//   different between macOS versions, and the text colour follows the system appearance
// - Windows draws buttons that have a custom colour in the classic 3D style (light outline) and
//   the combo box stays white with its own light border
//Drawing them here gives the same result everywhere and on every OS version.
//
//The controls use their own background/foreground colour (SetBackgroundColour/SetForegroundColour)
//and the background of their parent for the area behind the rounded corners. When the theme
//changes just set the colours again (that repaints them), same as with the native controls.
namespace ThemedControlsDetail
{
    //true when the colour is closer to black than to white
    inline bool IsDark(const wxColour& colour)
    {
        return (colour.Red() * 299 + colour.Green() * 587 + colour.Blue() * 114) / 1000 < 128;
    }

    inline wxColour Mix(const wxColour& a, const wxColour& b, int percentOfB)
    {
        return wxColour(
            static_cast<unsigned char>((a.Red() * (100 - percentOfB) + b.Red() * percentOfB) / 100),
            static_cast<unsigned char>((a.Green() * (100 - percentOfB) + b.Green() * percentOfB) / 100),
            static_cast<unsigned char>((a.Blue() * (100 - percentOfB) + b.Blue() * percentOfB) / 100));
    }

    struct Look
    {
        wxColour behind;
        wxColour fill;
        wxColour border;
        wxColour text;
    };

    inline Look GetLook(const wxWindow* window, bool enabled, bool hover, bool pressed)
    {
        const wxColour background = window->GetBackgroundColour();
        const wxColour foreground = window->GetForegroundColour();

        Look look;
        look.behind = window->GetParent() != nullptr ? window->GetParent()->GetBackgroundColour() : background;

        const bool darkTheme = IsDark(look.behind);
        if (pressed) {
            look.fill = background.ChangeLightness(darkTheme ? 85 : 90);
        } else if (hover) {
            look.fill = background.ChangeLightness(darkTheme ? 115 : 96);
        } else {
            look.fill = background;
        }
        look.border = background.ChangeLightness(darkTheme ? 78 : 85);
        look.text = enabled ? foreground : Mix(foreground, background, 55);
        return look;
    }

    //paints what is behind the control and then the rounded button shape
    inline void DrawFrame(wxGCDC& gdc, const wxWindow* window, const Look& look)
    {
        const wxSize size = window->GetClientSize();
        gdc.SetPen(*wxTRANSPARENT_PEN);
        gdc.SetBrush(wxBrush(look.behind));
        gdc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

        gdc.SetPen(wxPen(look.border, 1));
        gdc.SetBrush(wxBrush(look.fill));
        gdc.DrawRoundedRectangle(0, 0, size.GetWidth(), size.GetHeight(), window->FromDIP(5));
    }
}

//push button with a text and/or an icon
class ThemedButton : public wxControl
{
    public:
        ThemedButton(wxWindow* parent, wxWindowID id, const wxString& label)
        {
            wxControl::Create(parent, id, wxDefaultPosition, wxDefaultSize,
                wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE, wxDefaultValidator);
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            wxControl::SetLabel(label);

            Bind(wxEVT_PAINT, &ThemedButton::OnPaint, this);
            Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
            Bind(wxEVT_ENTER_WINDOW, &ThemedButton::OnEnter, this);
            Bind(wxEVT_LEAVE_WINDOW, &ThemedButton::OnLeave, this);
            Bind(wxEVT_MOTION, &ThemedButton::OnMotion, this);
            Bind(wxEVT_LEFT_DOWN, &ThemedButton::OnLeftDown, this);
            Bind(wxEVT_LEFT_DCLICK, &ThemedButton::OnLeftDown, this);
            Bind(wxEVT_LEFT_UP, &ThemedButton::OnLeftUp, this);
            Bind(wxEVT_MOUSE_CAPTURE_LOST, &ThemedButton::OnCaptureLost, this);
            SetInitialSize();
        }

        void SetLabel(const wxString& label) override
        {
            wxControl::SetLabel(label);
            InvalidateBestSize();
            Refresh();
        }

        //icon buttons (undo/redo). The icon is recoloured with the foreground colour of the button,
        //so a light icon can be used on the dark theme and still be visible on the light one.
        //This only works for icons with an alpha channel, other icons are drawn as they are.
        void SetBitmap(const wxBitmap& bitmap)
        {
            bitmap_ = bitmap;
            image_ = bitmap.IsOk() ? bitmap.ConvertToImage() : wxImage();
            tinted_ = wxBitmap();
            InvalidateBestSize();
            Refresh();
        }

        //clicking a button must not take the keyboard focus away from the editor
        bool AcceptsFocus() const override { return false; }

    protected:
        wxSize DoGetBestSize() const override
        {
            int width = 0;
            int height = 0;
            if (!GetLabel().IsEmpty()) {
                GetTextExtent(GetLabel(), &width, &height);
                width += 2 * FromDIP(14);
            }
            if (bitmap_.IsOk()) {
                width = std::max(width, bitmap_.GetWidth() + 2 * FromDIP(10));
                height = std::max(height, bitmap_.GetHeight());
            }
            height = std::max(height + 2 * FromDIP(6), FromDIP(28));
            return wxSize(width, height);
        }

    private:
        wxBitmap bitmap_;
        wxImage image_;
        mutable wxBitmap tinted_;
        mutable wxColour tintedFor_;
        bool hover_ = false;
        bool pressed_ = false;

        wxBitmap GetIcon(const wxColour& colour) const
        {
            if (!image_.IsOk() || !image_.HasAlpha()) {
                return bitmap_;
            }
            if (!tinted_.IsOk() || tintedFor_ != colour) {
                wxImage image = image_.Copy();
                unsigned char* data = image.GetData();
                const int pixels = image.GetWidth() * image.GetHeight();
                for (int i = 0; i < pixels; ++i) {
                    data[3 * i] = colour.Red();
                    data[3 * i + 1] = colour.Green();
                    data[3 * i + 2] = colour.Blue();
                }
                tinted_ = wxBitmap(image);
                tintedFor_ = colour;
            }
            return tinted_;
        }

        void OnPaint(wxPaintEvent&)
        {
            wxAutoBufferedPaintDC dc(this);
            wxGCDC gdc(dc);
            const ThemedControlsDetail::Look look =
                ThemedControlsDetail::GetLook(this, IsEnabled(), hover_, pressed_ && hover_);
            ThemedControlsDetail::DrawFrame(gdc, this, look);

            const wxSize size = GetClientSize();
            if (bitmap_.IsOk()) {
                const wxBitmap icon = GetIcon(look.text);
                gdc.DrawBitmap(icon, (size.GetWidth() - icon.GetWidth()) / 2,
                    (size.GetHeight() - icon.GetHeight()) / 2, true);
            }
            if (!GetLabel().IsEmpty()) {
                gdc.SetFont(GetFont());
                gdc.SetTextForeground(look.text);
                const wxSize text = gdc.GetTextExtent(GetLabel());
                gdc.DrawText(GetLabel(), (size.GetWidth() - text.GetWidth()) / 2,
                    (size.GetHeight() - text.GetHeight()) / 2);
            }
        }

        void OnEnter(wxMouseEvent& event)
        {
            hover_ = true;
            Refresh();
            event.Skip();
        }

        void OnLeave(wxMouseEvent& event)
        {
            //while the mouse button is held the mouse is captured, motion events keep hover_ up to date
            if (!HasCapture()) {
                hover_ = false;
                Refresh();
            }
            event.Skip();
        }

        void OnMotion(wxMouseEvent& event)
        {
            const bool inside = GetClientRect().Contains(event.GetPosition());
            if (inside != hover_) {
                hover_ = inside;
                Refresh();
            }
            event.Skip();
        }

        void OnLeftDown(wxMouseEvent&)
        {
            if (!IsEnabled()) {
                return;
            }
            pressed_ = true;
            hover_ = true;
            if (!HasCapture()) {
                CaptureMouse();
            }
            Refresh();
        }

        void OnLeftUp(wxMouseEvent& event)
        {
            if (HasCapture()) {
                ReleaseMouse();
            }
            const bool click = pressed_ && GetClientRect().Contains(event.GetPosition());
            pressed_ = false;
            hover_ = GetClientRect().Contains(event.GetPosition());
            Refresh();

            if (click) {
                wxCommandEvent clickEvent(wxEVT_BUTTON, GetId());
                clickEvent.SetEventObject(this);
                GetEventHandler()->ProcessEvent(clickEvent);
            }
        }

        void OnCaptureLost(wxMouseCaptureLostEvent&)
        {
            pressed_ = false;
            hover_ = false;
            Refresh();
        }
};

//dropdown list. It has the small part of the wxChoice interface the editor needs and sends
//wxEVT_CHOICE when the user picks an item (not when the selection is set from code, like wxChoice)
class ThemedChoice : public wxControl
{
    public:
        ThemedChoice(wxWindow* parent, wxWindowID id)
        {
            wxControl::Create(parent, id, wxDefaultPosition, wxDefaultSize,
                wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE, wxDefaultValidator);
            SetBackgroundStyle(wxBG_STYLE_PAINT);

            Bind(wxEVT_PAINT, &ThemedChoice::OnPaint, this);
            Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
            Bind(wxEVT_ENTER_WINDOW, &ThemedChoice::OnEnter, this);
            Bind(wxEVT_LEAVE_WINDOW, &ThemedChoice::OnLeave, this);
            Bind(wxEVT_LEFT_DOWN, &ThemedChoice::OnLeftDown, this);
            Bind(wxEVT_LEFT_DCLICK, &ThemedChoice::OnLeftDown, this);
            SetInitialSize();
        }

        int Append(const wxString& item)
        {
            items_.push_back(item);
            InvalidateBestSize();
            Refresh();
            return static_cast<int>(items_.size()) - 1;
        }

        unsigned int GetCount() const { return static_cast<unsigned int>(items_.size()); }

        wxString GetString(unsigned int index) const
        {
            return index < items_.size() ? items_[index] : wxString();
        }

        void SetSelection(int index)
        {
            selection_ = (index >= 0 && index < static_cast<int>(items_.size())) ? index : wxNOT_FOUND;
            Refresh();
        }

        int GetSelection() const { return selection_; }

        bool SetStringSelection(const wxString& item)
        {
            const std::vector<wxString>::const_iterator found = std::find(items_.begin(), items_.end(), item);
            if (found == items_.end()) {
                return false;
            }
            SetSelection(static_cast<int>(found - items_.begin()));
            return true;
        }

        wxString GetStringSelection() const { return GetString(static_cast<unsigned int>(selection_)); }

        bool AcceptsFocus() const override { return false; }

    protected:
        wxSize DoGetBestSize() const override
        {
            int width = 0;
            int height = 0;
            for (const wxString& item : items_) {
                int itemWidth = 0;
                GetTextExtent(item, &itemWidth, &height);
                width = std::max(width, itemWidth);
            }
            width += FromDIP(14) + FromDIP(28); //left padding and the space for the arrow
            height = std::max(height + 2 * FromDIP(6), FromDIP(28));
            return wxSize(width, height);
        }

    private:
        static const int firstItemId_ = 12000;
        std::vector<wxString> items_;
        int selection_ = wxNOT_FOUND;
        bool hover_ = false;

        void OnPaint(wxPaintEvent&)
        {
            wxAutoBufferedPaintDC dc(this);
            wxGCDC gdc(dc);
            const ThemedControlsDetail::Look look = ThemedControlsDetail::GetLook(this, IsEnabled(), hover_, false);
            ThemedControlsDetail::DrawFrame(gdc, this, look);

            const wxSize size = GetClientSize();
            gdc.SetFont(GetFont());
            gdc.SetTextForeground(look.text);
            const wxString text = GetStringSelection();
            const wxSize textSize = gdc.GetTextExtent(text);
            gdc.DrawText(text, FromDIP(14), (size.GetHeight() - textSize.GetHeight()) / 2);

            //arrow
            const int arrowX = size.GetWidth() - FromDIP(16);
            const int arrowY = size.GetHeight() / 2;
            const int half = FromDIP(4);
            wxPoint arrow[3] = {
                wxPoint(arrowX - half, arrowY - half / 2),
                wxPoint(arrowX, arrowY + half / 2 + 1),
                wxPoint(arrowX + half, arrowY - half / 2)
            };
            gdc.SetPen(wxPen(look.text, std::max(1, FromDIP(1))));
            gdc.DrawLines(3, arrow);
        }

        void OnEnter(wxMouseEvent& event)
        {
            hover_ = true;
            Refresh();
            event.Skip();
        }

        void OnLeave(wxMouseEvent& event)
        {
            hover_ = false;
            Refresh();
            event.Skip();
        }

        void OnLeftDown(wxMouseEvent&)
        {
            if (!IsEnabled() || items_.empty()) {
                return;
            }

            wxMenu menu;
            for (size_t i = 0; i < items_.size(); ++i) {
                wxMenuItem* item = menu.AppendRadioItem(firstItemId_ + static_cast<int>(i), items_[i]);
                if (static_cast<int>(i) == selection_) {
                    item->Check(true);
                }
            }
            menu.Bind(wxEVT_MENU, &ThemedChoice::OnMenuItem, this);

            hover_ = false;
            Refresh();
            PopupMenu(&menu, 0, GetSize().GetHeight());
        }

        void OnMenuItem(wxCommandEvent& event)
        {
            const int index = event.GetId() - firstItemId_;
            if (index < 0 || index >= static_cast<int>(items_.size()) || index == selection_) {
                return;
            }

            SetSelection(index);
            wxCommandEvent choiceEvent(wxEVT_CHOICE, GetId());
            choiceEvent.SetEventObject(this);
            choiceEvent.SetInt(index);
            choiceEvent.SetString(items_[index]);
            GetEventHandler()->ProcessEvent(choiceEvent);
        }
};