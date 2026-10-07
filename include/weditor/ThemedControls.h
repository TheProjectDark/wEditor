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
#include <wx/stopwatch.h>
#include <wx/timer.h>
#include <wx/weakref.h>
#include <algorithm>
#include <functional>
#include <utility>
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
//
//The look is flat and minimal: a hairline outline, soft 6 DIP corners, no gradients or 3D edges.
//Hover and press need no extra theme colours, they tint the background towards the text colour,
//so the same code works on light and dark themes. The hover tint fades in and out (kFadeMs).
namespace ThemedControlsDetail
{
    //how long the hover highlight takes to fade in or out, in milliseconds (0 turns the fade off)
    const int kFadeMs = 120;
    //timer period of the fade, about 60 frames per second
    const int kFrameMs = 16;

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

    //accent colour of the primary button (the same blue as the active tab line)
    inline wxColour Accent()
    {
        return wxColour(0, 122, 204);
    }

    struct Look
    {
        wxColour behind;
        wxColour fill;
        wxColour border;
        wxColour text;
    };

    //hoverPercent is how far the hover highlight is faded in, 0..100
    inline Look GetLook(const wxWindow* window, bool enabled, int hoverPercent, bool pressed, bool primary = false)
    {
        const wxColour background = window->GetBackgroundColour();
        const wxColour foreground = window->GetForegroundColour();

        Look look;
        look.behind = window->GetParent() != nullptr ? window->GetParent()->GetBackgroundColour() : background;

        if (primary) {
            const wxColour white(255, 255, 255);
            if (enabled) {
                look.fill = pressed ? Accent().ChangeLightness(85)
                                    : Accent().ChangeLightness(100 + hoverPercent * 12 / 100);
                look.text = white;
            } else {
                look.fill = Mix(background, Accent(), 40);
                look.text = Mix(white, look.fill, 40);
            }
            look.border = look.fill;
            return look;
        }

        if (!enabled) {
            look.fill = background;
            look.border = Mix(background, foreground, 8);
            look.text = Mix(foreground, background, 55);
            return look;
        }

        //hover and press only tint the background towards the text colour
        look.fill = Mix(background, foreground, pressed ? 14 : hoverPercent * 8 / 100);
        look.border = Mix(background, foreground, pressed ? 28 : 14 + hoverPercent * 10 / 100);
        look.text = foreground;
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
        gdc.DrawRoundedRectangle(0, 0, size.GetWidth(), size.GetHeight(), window->FromDIP(6));
    }

    //Fades the hover highlight in and out, like `transition: 120ms` in CSS. A control keeps one of
    //these, calls SetTarget() when the pointer enters or leaves and reads Percent() when it paints.
    class HoverFade : public wxEvtHandler
    {
        public:
            explicit HoverFade(wxWindow* window)
                : window_(window), timer_(this)
            {
                Bind(wxEVT_TIMER, &HoverFade::OnTimer, this);
            }

            void SetTarget(bool on)
            {
                target_ = on ? 1.0 : 0.0;
                if (kFadeMs <= 0) {
                    level_ = target_;
                } else if (level_ != target_ && !timer_.IsRunning()) {
                    lastTick_ = watch_.Time();
                    timer_.Start(kFrameMs);
                }
            }

            //0..100
            int Percent() const
            {
                return static_cast<int>(level_ * 100.0 + 0.5);
            }

        private:
            wxWindow* window_;
            wxTimer timer_;
            wxStopWatch watch_;
            double level_ = 0.0;
            double target_ = 0.0;
            long lastTick_ = 0;

            void OnTimer(wxTimerEvent&)
            {
                const long now = watch_.Time();
                const double step = static_cast<double>(now - lastTick_) / kFadeMs;
                lastTick_ = now;
                level_ = level_ < target_ ? std::min(target_, level_ + step)
                                          : std::max(target_, level_ - step);
                if (level_ == target_) {
                    timer_.Stop();
                }
                window_->Refresh();
            }
    };
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

        //filled with the accent colour and white text, for the main action of a panel
        void SetPrimary(bool primary)
        {
            if (primary_ != primary) {
                primary_ = primary;
                Refresh();
            }
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
        bool primary_ = false;
        ThemedControlsDetail::HoverFade fade_{this};

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

        //repaints and starts the fade, but only when the state really changed
        void SetHover(bool hover)
        {
            if (hover_ != hover) {
                hover_ = hover;
                fade_.SetTarget(hover);
                Refresh();
            }
        }

        void OnPaint(wxPaintEvent&)
        {
            wxAutoBufferedPaintDC dc(this);
            wxGCDC gdc(dc);
            const ThemedControlsDetail::Look look = ThemedControlsDetail::GetLook(
                this, IsEnabled(), fade_.Percent(), pressed_ && hover_, primary_);
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
            SetHover(true);
            event.Skip();
        }

        void OnLeave(wxMouseEvent& event)
        {
            //while the mouse button is held the mouse is captured, motion events keep hover_ up to date
            if (!HasCapture()) {
                SetHover(false);
            }
            event.Skip();
        }

        void OnMotion(wxMouseEvent& event)
        {
            SetHover(GetClientRect().Contains(event.GetPosition()));
            event.Skip();
        }

        void OnLeftDown(wxMouseEvent&)
        {
            if (!IsEnabled()) {
                return;
            }
            pressed_ = true;
            SetHover(true);
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
            const bool inside = GetClientRect().Contains(event.GetPosition());
            const bool click = pressed_ && inside;
            pressed_ = false;
            SetHover(inside);
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
            SetHover(false);
            Refresh();
        }
};

//Compact editor tab with its own active, dirty, hover, and close affordances.
class ThemedTabButton : public wxControl
{
    public:
        using CloseHandler = std::function<void()>;

        ThemedTabButton(wxWindow* parent, wxWindowID id, const wxString& label)
        {
            wxControl::Create(parent, id, wxDefaultPosition, wxDefaultSize,
                wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE, wxDefaultValidator);
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            wxControl::SetLabel(label);

            Bind(wxEVT_PAINT, &ThemedTabButton::OnPaint, this);
            Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
            Bind(wxEVT_ENTER_WINDOW, &ThemedTabButton::OnEnter, this);
            Bind(wxEVT_LEAVE_WINDOW, &ThemedTabButton::OnLeave, this);
            Bind(wxEVT_MOTION, &ThemedTabButton::OnMotion, this);
            Bind(wxEVT_LEFT_DOWN, &ThemedTabButton::OnLeftDown, this);
            Bind(wxEVT_LEFT_DCLICK, &ThemedTabButton::OnLeftDown, this);
            Bind(wxEVT_LEFT_UP, &ThemedTabButton::OnLeftUp, this);
            Bind(wxEVT_MOUSE_CAPTURE_LOST, &ThemedTabButton::OnCaptureLost, this);
            SetInitialSize();
        }

        void SetLabel(const wxString& label) override
        {
            if (GetLabel() == label) {
                return;
            }
            wxControl::SetLabel(label);
            InvalidateBestSize();
            Refresh();
        }

        void SetActive(bool active)
        {
            if (active_ != active) {
                active_ = active;
                Refresh();
            }
        }

        void SetModified(bool modified)
        {
            if (modified_ != modified) {
                modified_ = modified;
                Refresh();
            }
        }

        void SetCloseHandler(CloseHandler handler)
        {
            closeHandler_ = std::move(handler);
        }

        bool AcceptsFocus() const override { return false; }

    protected:
        //The width follows the label. The min/max width live here and not in SetMinSize/SetMaxSize
        //on purpose: an explicit min size with both dimensions set makes the sizer ignore the best
        //size, so every tab would stay at that width no matter how long the file name is.
        wxSize DoGetBestSize() const override
        {
            int width = 0;
            int height = 0;
            GetTextExtent(GetLabel(), &width, &height);
            width += FromDIP(10) + FromDIP(10) + FromDIP(26); //left padding, gap, close button
            width = std::min(std::max(width, FromDIP(64)), FromDIP(220));
            height = std::max(height + FromDIP(8), FromDIP(26));
            return wxSize(width, height);
        }

    private:
        CloseHandler closeHandler_;
        bool active_ = false;
        bool modified_ = false;
        bool hover_ = false;
        bool pressed_ = false;
        bool pressedClose_ = false;

        wxRect CloseRect() const
        {
            const wxSize size = GetClientSize();
            const int closeWidth = FromDIP(26);
            return wxRect(size.GetWidth() - closeWidth, 0, closeWidth, size.GetHeight());
        }

        void OnPaint(wxPaintEvent&)
        {
            wxAutoBufferedPaintDC dc(this);
            wxGCDC gdc(dc);
            const wxSize size = GetClientSize();
            const wxColour behind = GetParent() != nullptr
                ? GetParent()->GetBackgroundColour() : GetBackgroundColour();
            wxColour fill = active_ ? GetBackgroundColour() : behind;
            const wxColour foreground = GetForegroundColour();
            if (pressed_ || (hover_ && !active_)) {
                fill = ThemedControlsDetail::Mix(fill, foreground, pressed_ ? 9 : 7);
            }

            gdc.SetPen(*wxTRANSPARENT_PEN);
            gdc.SetBrush(wxBrush(fill));
            gdc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

            const wxColour separator = ThemedControlsDetail::Mix(behind, foreground, 13);
            if (active_) {
                gdc.SetPen(wxPen(wxColour(0, 122, 204), FromDIP(2)));
                gdc.DrawLine(0, FromDIP(1), size.GetWidth(), FromDIP(1));
            }
            gdc.SetPen(wxPen(separator, 1));
            gdc.DrawLine(size.GetWidth() - 1, FromDIP(5), size.GetWidth() - 1,
                         size.GetHeight() - FromDIP(5));

            gdc.SetFont(GetFont());
            const wxColour labelColour = active_
                ? foreground : ThemedControlsDetail::Mix(foreground, behind, 24);
            gdc.SetTextForeground(labelColour);
            const int maxTextWidth = size.GetWidth() - FromDIP(10) - FromDIP(10) - FromDIP(26);
            const wxString label = wxControl::Ellipsize(GetLabel(), gdc, wxELLIPSIZE_END, maxTextWidth);
            const wxSize textSize = gdc.GetTextExtent(label);
            gdc.DrawText(label, FromDIP(10), (size.GetHeight() - textSize.GetHeight()) / 2);

            const wxRect closeRect = CloseRect();
            const wxPoint center = closeRect.GetPosition() +
                wxPoint(closeRect.GetWidth() / 2, closeRect.GetHeight() / 2);
            if (hover_ || (active_ && !modified_)) {
                const int radius = FromDIP(4);
                gdc.SetPen(wxPen(labelColour, FromDIP(1)));
                gdc.DrawLine(center.x - radius, center.y - radius,
                             center.x + radius, center.y + radius);
                gdc.DrawLine(center.x + radius, center.y - radius,
                             center.x - radius, center.y + radius);
            } else if (modified_) {
                gdc.SetPen(*wxTRANSPARENT_PEN);
                gdc.SetBrush(wxBrush(wxColour(0, 122, 204)));
                gdc.DrawCircle(center, FromDIP(3));
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

        void OnLeftDown(wxMouseEvent& event)
        {
            if (!IsEnabled()) {
                return;
            }
            pressed_ = true;
            pressedClose_ = CloseRect().Contains(event.GetPosition());
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
            const bool inside = GetClientRect().Contains(event.GetPosition());
            const bool close = pressed_ && pressedClose_ && CloseRect().Contains(event.GetPosition());
            const bool activate = pressed_ && !pressedClose_ && inside;
            pressed_ = false;
            pressedClose_ = false;
            hover_ = inside;
            Refresh();

            if (close && closeHandler_) {
                closeHandler_();
            } else if (activate) {
                wxCommandEvent clickEvent(wxEVT_BUTTON, GetId());
                clickEvent.SetEventObject(this);
                GetEventHandler()->ProcessEvent(clickEvent);
            }
        }

        void OnCaptureLost(wxMouseCaptureLostEvent&)
        {
            pressed_ = false;
            pressedClose_ = false;
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
        bool open_ = false; //the list is shown, the control stays highlighted until it closes
        ThemedControlsDetail::HoverFade fade_{this};

        //repaints and starts the fade, but only when the state really changed
        void SetHover(bool hover)
        {
            if (hover_ != hover) {
                hover_ = hover;
                fade_.SetTarget(hover);
                Refresh();
            }
        }

        void OnPaint(wxPaintEvent&)
        {
            wxAutoBufferedPaintDC dc(this);
            wxGCDC gdc(dc);
            const int hover = fade_.Percent();
            const ThemedControlsDetail::Look look = ThemedControlsDetail::GetLook(this, IsEnabled(), hover, open_);
            ThemedControlsDetail::DrawFrame(gdc, this, look);

            const wxSize size = GetClientSize();
            gdc.SetFont(GetFont());
            gdc.SetTextForeground(look.text);
            const wxString text = GetStringSelection();
            const wxSize textSize = gdc.GetTextExtent(text);
            gdc.DrawText(text, FromDIP(14), (size.GetHeight() - textSize.GetHeight()) / 2);

            //chevron: thin, with round ends, muted until the pointer is over the control or the list is open
            const int arrowX = size.GetWidth() - FromDIP(16);
            const int arrowY = size.GetHeight() / 2;
            const int half = FromDIP(4);
            wxPoint arrow[3] = {
                wxPoint(arrowX - half, arrowY - half / 2),
                wxPoint(arrowX, arrowY + half / 2),
                wxPoint(arrowX + half, arrowY - half / 2)
            };
            const int emphasis = open_ ? 100 : hover;
            wxPen pen(ThemedControlsDetail::Mix(look.fill, look.text, 60 + emphasis * 40 / 100),
                std::max(1, FromDIP(1)));
            pen.SetCap(wxCAP_ROUND);
            pen.SetJoin(wxJOIN_ROUND);
            gdc.SetPen(pen);
            gdc.DrawLines(3, arrow);
        }

        void OnEnter(wxMouseEvent& event)
        {
            SetHover(true);
            event.Skip();
        }

        void OnLeave(wxMouseEvent& event)
        {
            SetHover(false);
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

            open_ = true;
            SetHover(false);
            Refresh();
            Update();

            //the handler of wxEVT_CHOICE may delete this control while the menu is still running
            wxWeakRef<ThemedChoice> self(this);
            PopupMenu(&menu, 0, GetSize().GetHeight());
            if (!self) {
                return;
            }

            open_ = false;
            SetHover(GetScreenRect().Contains(wxGetMousePosition()));
            Refresh();
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