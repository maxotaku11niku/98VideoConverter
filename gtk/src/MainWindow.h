// 98VideoConverter: Converts videos into .98v format, suitable for use with 98VIDEOP.COM
// Maxim Hoxha 2022-2023
// This is the main application window
// This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

#pragma once

#include <gtkmm/window.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>

class MainWindow : public Gtk::ApplicationWindow
{
public:
    MainWindow();
    virtual ~MainWindow();

protected:
    Glib::RefPtr<Gtk::Builder> builderRef;
};