// 98VideoConverter: Converts videos into .98v format, suitable for use with 98VIDEOP.COM
// Maxim Hoxha 2022-2023
// This is the main application root
// This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

#pragma once

#include <glibmm.h>
#include <gtkmm/application.h>
#include <gtkmm/builder.h>
#include <gtkmm/aboutdialog.h>
#include "ConverterEngine.h"
#include "MainWindow.h"

class VideoConverter98 : public Gtk::Application
{
public:
    static Glib::RefPtr<VideoConverter98> create();
    inline VideoConverterEngine* getConv() { return conv; };

    bool EncodeProgressCallback(unsigned int frameNum);

protected:
    VideoConverter98();

    void on_startup() override;
    void on_activate() override;

private:
    void CreateWindow();

    void OnHideWindow(Gtk::Window* window);
    void OnMenuFileOpen();
    void OnMenuFileExport();
    void OnMenuFileQuit();
    void OnMenuEditSettings();
    void OnMenuEditPalette();
    void OnMenuHelpAbout();
    void OnAboutDialogResponse(int responseID);

    Glib::RefPtr<Gtk::Builder> builderRef;
    Gtk::AboutDialog aboutDialog;
    MainWindow* mwin;
    VideoConverterEngine* conv;
};
