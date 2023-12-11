// 98VideoConverter: Converts videos into .98v format, suitable for use with 98VIDEOP.COM
// Maxim Hoxha 2022-2023
// This is the main application window
// This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

#pragma once

#include <glibmm.h>
#include <gtkmm/window.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/image.h>
#include <gtkmm/scale.h>
#include <gtkmm/spinbutton.h>
#include <gdkmm/pixbuf.h>
#include "ConverterEngine.h"

class MainWindow : public Gtk::ApplicationWindow
{
public:
    MainWindow();
    virtual ~MainWindow();

    void DisableConfig();
    void EnableConfig();
    void SetEditView();
    void RollingUpdateViewUpdate();

    VideoConverterEngine* conv;

protected:
    Glib::RefPtr<Gtk::Builder> builderRef;

    Glib::RefPtr<Gtk::Adjustment> frameNum;
    Glib::RefPtr<Gtk::Adjustment> bitrate;
    Glib::RefPtr<Gtk::Adjustment> lumdither;
    Glib::RefPtr<Gtk::Adjustment> satdither;
    Glib::RefPtr<Gtk::Adjustment> huedither;
    Glib::RefPtr<Gtk::Adjustment> uvbias;

    Gtk::Image* beforeImage;
    Gtk::Image* afterImage;

    Gtk::Scale* frameSeeker;

    Gtk::SpinButton* bitrateSelect;
    Gtk::SpinButton* lumditSelect;
    Gtk::SpinButton* satditSelect;
    Gtk::SpinButton* hueditSelect;
    Gtk::SpinButton* uvbiasSelect;

private:
    void OnSeekFrame();
    void OnChangeBitrate();
    void OnChangeLumDither();
    void OnChangeSatDither();
    void OnChangeHueDither();
    void OnChangeUVBias();

    Glib::RefPtr<Gdk::Pixbuf> beforePixbuf;
    Glib::RefPtr<Gdk::Pixbuf> afterPixbuf;
};
