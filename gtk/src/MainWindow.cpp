// 98VideoConverter: Converts videos into .98v format, suitable for use with 98VIDEOP.COM
// Maxim Hoxha 2022-2023
// This is the main application window
// This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

#include <gtkmm/box.h>
#include <string.h>
#include "MainWindow.h"

//All GUI construction is coded into the binary because this is a small and simple (conceptually) program.

static const char* uiXML =
"<interface>"
    "<object class='GtkAdjustment' id='frameNum'>"
        "<property name='upper'>1000</property>"
        "<property name='step-increment'>1</property>"
        "<property name='page-increment'>10</property>"
    "</object>"
    "<object class='GtkAdjustment' id='bitrate'>"
        "<property name='upper'>1000000</property>"
        "<property name='step-increment'>1</property>"
        "<property name='page-increment'>10</property>"
    "</object>"
    "<object class='GtkAdjustment' id='lumdither'>"
        "<property name='upper'>2</property>"
        "<property name='step-increment'>0.001</property>"
        "<property name='page-increment'>0.01</property>"
    "</object>"
    "<object class='GtkAdjustment' id='satdither'>"
        "<property name='upper'>2</property>"
        "<property name='step-increment'>0.001</property>"
        "<property name='page-increment'>0.01</property>"
    "</object>"
    "<object class='GtkAdjustment' id='huedither'>"
        "<property name='upper'>10</property>"
        "<property name='step-increment'>0.001</property>"
        "<property name='page-increment'>0.01</property>"
    "</object>"
    "<object class='GtkAdjustment' id='uvbias'>"
        "<property name='lower'>0.1</property>"
        "<property name='upper'>4</property>"
        "<property name='step-increment'>0.001</property>"
        "<property name='page-increment'>0.01</property>"
    "</object>"
    "<object class='GtkBox' id='mainView'>"
        "<property name='visible'>True</property>"
        "<property name='can-focus'>False</property>"
        "<property name='orientation'>vertical</property>"
        "<child>"
            "<object class='GtkBox'>"
                "<property name='visible'>True</property>"
                "<property name='can-focus'>False</property>"
                "<child>"
                    "<object class='GtkImage' id='beforeImage'>"
                        "<property name='visible'>True</property>"
                        "<property name='can-focus'>False</property>"
                    "</object>"
                    "<packing>"
                        "<property name='expand'>True</property>"
                        "<property name='fill'>True</property>"
                        "<property name='position'>0</property>"
                    "</packing>"
                "</child>"
                "<child>"
                    "<object class='GtkImage' id='afterImage'>"
                        "<property name='visible'>True</property>"
                        "<property name='can-focus'>False</property>"
                    "</object>"
                    "<packing>"
                        "<property name='expand'>True</property>"
                        "<property name='fill'>True</property>"
                        "<property name='position'>1</property>"
                    "</packing>"
                "</child>"
            "</object>"
            "<packing>"
                "<property name='expand'>True</property>"
                "<property name='fill'>True</property>"
                "<property name='position'>0</property>"
            "</packing>"
        "</child>"
        "<child>"
            "<object class='GtkScale' id='frameSeeker'>"
                "<property name='visible'>True</property>"
                "<property name='can-focus'>True</property>"
                "<property name='adjustment'>frameNum</property>"
                "<property name='round-digits'>1</property>"
            "</object>"
            "<packing>"
                "<property name='expand'>False</property>"
                "<property name='fill'>True</property>"
                "<property name='position'>1</property>"
            "</packing>"
        "</child>"
        "<child>"
            "<object class='GtkBox'>"
                "<property name='visible'>True</property>"
                "<property name='can-focus'>False</property>"
                "<child>"
                    "<object class='GtkBox'>"
                        "<property name='visible'>True</property>"
                        "<property name='can-focus'>False</property>"
                        "<property name='orientation'>vertical</property>"
                        "<child>"
                            "<object class='GtkLabel'>"
                                "<property name='visible'>True</property>"
                                "<property name='can-focus'>False</property>"
                                "<property name='label' translatable='yes'>Target Bitrate (bytes/frame)</property>"
                            "</object>"
                            "<packing>"
                                "<property name='expand'>False</property>"
                                "<property name='fill'>True</property>"
                                "<property name='position'>0</property>"
                            "</packing>"
                        "</child>"
                        "<child>"
                            "<object class='GtkSpinButton' id='bitrateSelect'>"
                                "<property name='visible'>True</property>"
                                "<property name='can-focus'>True</property>"
                                "<property name='input-purpose'>number</property>"
                                "<property name='adjustment'>bitrate</property>"
                                "<property name='climb-rate'>1</property>"
                                "<property name='numeric'>True</property>"
                            "</object>"
                            "<packing>"
                                "<property name='expand'>False</property>"
                                "<property name='fill'>True</property>"
                                "<property name='position'>1</property>"
                            "</packing>"
                        "</child>"
                    "</object>"
                    "<packing>"
                        "<property name='expand'>True</property>"
                        "<property name='fill'>True</property>"
                        "<property name='padding'>4</property>"
                        "<property name='position'>0</property>"
                    "</packing>"
                "</child>"
                "<child>"
                    "<object class='GtkBox'>"
                        "<property name='visible'>True</property>"
                        "<property name='can-focus'>False</property>"
                        "<property name='orientation'>vertical</property>"
                        "<child>"
                            "<object class='GtkLabel'>"
                                "<property name='visible'>True</property>"
                                "<property name='can-focus'>False</property>"
                                "<property name='label' translatable='yes'>Luminosity Dither Factor</property>"
                            "</object>"
                            "<packing>"
                                "<property name='expand'>False</property>"
                                "<property name='fill'>True</property>"
                                "<property name='position'>0</property>"
                            "</packing>"
                        "</child>"
                        "<child>"
                            "<object class='GtkSpinButton' id='lumditSelect'>"
                                "<property name='visible'>True</property>"
                                "<property name='can-focus'>True</property>"
                                "<property name='input-purpose'>number</property>"
                                "<property name='adjustment'>lumdither</property>"
                                "<property name='climb-rate'>0.01</property>"
                                "<property name='digits'>3</property>"
                                "<property name='numeric'>True</property>"
                            "</object>"
                            "<packing>"
                                "<property name='expand'>False</property>"
                                "<property name='fill'>True</property>"
                                "<property name='position'>1</property>"
                            "</packing>"
                        "</child>"
                    "</object>"
                    "<packing>"
                        "<property name='expand'>True</property>"
                        "<property name='fill'>True</property>"
                        "<property name='padding'>4</property>"
                        "<property name='position'>1</property>"
                    "</packing>"
                "</child>"
                "<child>"
                    "<object class='GtkBox'>"
                        "<property name='visible'>True</property>"
                        "<property name='can-focus'>False</property>"
                        "<property name='orientation'>vertical</property>"
                        "<child>"
                            "<object class='GtkLabel'>"
                                "<property name='visible'>True</property>"
                                "<property name='can-focus'>False</property>"
                                "<property name='label' translatable='yes'>Saturation Dither Factor</property>"
                            "</object>"
                            "<packing>"
                                "<property name='expand'>False</property>"
                                "<property name='fill'>True</property>"
                                "<property name='position'>0</property>"
                            "</packing>"
                        "</child>"
                        "<child>"
                            "<object class='GtkSpinButton' id='satditSelect'>"
                                "<property name='visible'>True</property>"
                                "<property name='can-focus'>True</property>"
                                "<property name='input-purpose'>number</property>"
                                "<property name='adjustment'>satdither</property>"
                                "<property name='climb-rate'>0.01</property>"
                                "<property name='digits'>3</property>"
                                "<property name='numeric'>True</property>"
                            "</object>"
                            "<packing>"
                                "<property name='expand'>False</property>"
                                "<property name='fill'>True</property>"
                                "<property name='position'>1</property>"
                            "</packing>"
                        "</child>"
                    "</object>"
                    "<packing>"
                        "<property name='expand'>True</property>"
                        "<property name='fill'>True</property>"
                        "<property name='padding'>4</property>"
                        "<property name='position'>2</property>"
                    "</packing>"
                "</child>"
                "<child>"
                    "<object class='GtkBox'>"
                        "<property name='visible'>True</property>"
                        "<property name='can-focus'>False</property>"
                        "<property name='orientation'>vertical</property>"
                        "<child>"
                            "<object class='GtkLabel'>"
                                "<property name='visible'>True</property>"
                                "<property name='can-focus'>False</property>"
                                "<property name='label' translatable='yes'>Hue Dither Factor</property>"
                            "</object>"
                            "<packing>"
                                "<property name='expand'>False</property>"
                                "<property name='fill'>True</property>"
                                "<property name='position'>0</property>"
                            "</packing>"
                        "</child>"
                        "<child>"
                            "<object class='GtkSpinButton' id='hueditSelect'>"
                                "<property name='visible'>True</property>"
                                "<property name='can-focus'>True</property>"
                                "<property name='input-purpose'>number</property>"
                                "<property name='adjustment'>huedither</property>"
                                "<property name='climb-rate'>0.01</property>"
                                "<property name='digits'>3</property>"
                                "<property name='numeric'>True</property>"
                            "</object>"
                            "<packing>"
                                "<property name='expand'>False</property>"
                                "<property name='fill'>True</property>"
                                "<property name='position'>1</property>"
                            "</packing>"
                        "</child>"
                    "</object>"
                    "<packing>"
                        "<property name='expand'>True</property>"
                        "<property name='fill'>True</property>"
                        "<property name='padding'>4</property>"
                        "<property name='position'>3</property>"
                    "</packing>"
                "</child>"
                "<child>"
                    "<object class='GtkBox'>"
                        "<property name='visible'>True</property>"
                        "<property name='can-focus'>False</property>"
                        "<property name='orientation'>vertical</property>"
                        "<child>"
                            "<object class='GtkLabel'>"
                                "<property name='visible'>True</property>"
                                "<property name='can-focus'>False</property>"
                                "<property name='label' translatable='yes'>UV Bias Factor</property>"
                            "</object>"
                            "<packing>"
                                "<property name='expand'>False</property>"
                                "<property name='fill'>True</property>"
                                "<property name='position'>0</property>"
                            "</packing>"
                        "</child>"
                        "<child>"
                            "<object class='GtkSpinButton' id='uvbiasSelect'>"
                                "<property name='visible'>True</property>"
                                "<property name='can-focus'>True</property>"
                                "<property name='input-purpose'>number</property>"
                                "<property name='adjustment'>uvbias</property>"
                                "<property name='climb-rate'>0.01</property>"
                                "<property name='digits'>3</property>"
                                "<property name='numeric'>True</property>"
                                "<property name='value'>1</property>"
                            "</object>"
                            "<packing>"
                                "<property name='expand'>False</property>"
                                "<property name='fill'>True</property>"
                                "<property name='position'>1</property>"
                            "</packing>"
                        "</child>"
                    "</object>"
                    "<packing>"
                        "<property name='expand'>True</property>"
                        "<property name='fill'>True</property>"
                        "<property name='padding'>4</property>"
                        "<property name='position'>4</property>"
                    "</packing>"
                "</child>"
            "</object>"
            "<packing>"
                "<property name='expand'>False</property>"
                "<property name='fill'>True</property>"
                "<property name='position'>2</property>"
            "</packing>"
        "</child>"
    "</object>"
"</interface>";

MainWindow::MainWindow() : Gtk::ApplicationWindow()
{
    set_title("98VideoConverter");
    set_default_size(1280, 600);
    builderRef = Gtk::Builder::create();
    builderRef->add_from_string(uiXML);

    Gtk::Box* mainView;
    builderRef->get_widget<Gtk::Box>("mainView", mainView);
    add(*mainView);

    Glib::RefPtr<Glib::Object> uiobj = builderRef->get_object("frameNum");
    frameNum = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(uiobj);
    uiobj = builderRef->get_object("bitrate");
    bitrate = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(uiobj);
    uiobj = builderRef->get_object("lumdither");
    lumdither = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(uiobj);
    uiobj = builderRef->get_object("satdither");
    satdither = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(uiobj);
    uiobj = builderRef->get_object("huedither");
    huedither = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(uiobj);
    uiobj = builderRef->get_object("uvbias");
    uvbias = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(uiobj);

    frameNum->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::OnSeekFrame));
    bitrate->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::OnChangeBitrate));
    lumdither->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::OnChangeLumDither));
    satdither->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::OnChangeSatDither));
    huedither->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::OnChangeHueDither));
    uvbias->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::OnChangeUVBias));

    builderRef->get_widget<Gtk::Image>("beforeImage", beforeImage);
    builderRef->get_widget<Gtk::Image>("afterImage", afterImage);
    builderRef->get_widget<Gtk::Scale>("frameSeeker", frameSeeker);
    builderRef->get_widget<Gtk::SpinButton>("bitrateSelect", bitrateSelect);
    builderRef->get_widget<Gtk::SpinButton>("lumditSelect", lumditSelect);
    builderRef->get_widget<Gtk::SpinButton>("satditSelect", satditSelect);
    builderRef->get_widget<Gtk::SpinButton>("hueditSelect", hueditSelect);
    builderRef->get_widget<Gtk::SpinButton>("uvbiasSelect", uvbiasSelect);

    beforePixbuf = Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB, true, 8, 640, 400);
    afterPixbuf = Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB, true, 8, 640, 400);
    beforePixbuf->fill(0x000000FF);
    afterPixbuf->fill(0x000000FF);
    beforeImage->set(beforePixbuf);
    afterImage->set(afterPixbuf);

    DisableConfig();
}

MainWindow::~MainWindow()
{
    
}

void MainWindow::DisableConfig()
{
    frameSeeker->set_sensitive(false);
    bitrateSelect->set_sensitive(false);
    lumditSelect->set_sensitive(false);
    satditSelect->set_sensitive(false);
    hueditSelect->set_sensitive(false);
    uvbiasSelect->set_sensitive(false);
}

void MainWindow::EnableConfig()
{
    frameSeeker->set_sensitive(true);
    bitrateSelect->set_sensitive(true);
    lumditSelect->set_sensitive(true);
    satditSelect->set_sensitive(true);
    hueditSelect->set_sensitive(true);
    uvbiasSelect->set_sensitive(true);
}

void MainWindow::SetEditView()
{
    memcpy(afterPixbuf->get_pixels(), conv->GrabFrame(0), PC_98_RESOLUTION * 4);
    memcpy(beforePixbuf->get_pixels(), conv->GetRescaledInputFrame(), PC_98_RESOLUTION * 4);

    frameNum->set_upper(conv->GetOrigFrameNumber());
    frameNum->set_value(0.0);
    bitrate->set_value(conv->GetBitrate());
    lumdither->set_value(conv->GetDitherFactor());
    satdither->set_value(conv->GetSaturationDitherFactor());
    huedither->set_value(conv->GetHueDitherFactor());
    uvbias->set_value(conv->GetUVBias());

    EnableConfig();
}

void MainWindow::RollingUpdateViewUpdate()
{
    memcpy(afterPixbuf->get_pixels(), conv->GetSimulatedOutput(), PC_98_RESOLUTION * 4);
    memcpy(beforePixbuf->get_pixels(), conv->GetRescaledInputFrame(), PC_98_RESOLUTION * 4);
    beforeImage->set(beforePixbuf);
    afterImage->set(afterPixbuf);
}


void MainWindow::OnSeekFrame()
{
    double fltFrame = frameNum->get_value();
    memcpy(afterPixbuf->get_pixels(), conv->GrabFrame((int)fltFrame), PC_98_RESOLUTION * 4);
    memcpy(beforePixbuf->get_pixels(), conv->GetRescaledInputFrame(), PC_98_RESOLUTION * 4);
    beforeImage->set(beforePixbuf);
    afterImage->set(afterPixbuf);
}

void MainWindow::OnChangeBitrate()
{
    conv->SetBitrate(bitrate->get_value());
}

void MainWindow::OnChangeLumDither()
{
    conv->SetDitherFactor(lumdither->get_value());
    memcpy(afterPixbuf->get_pixels(), conv->ReprocessGrabbedFrame(), PC_98_RESOLUTION * 4);
    afterImage->set(afterPixbuf);
}

void MainWindow::OnChangeSatDither()
{
    conv->SetSaturationDitherFactor(satdither->get_value());
    memcpy(afterPixbuf->get_pixels(), conv->ReprocessGrabbedFrame(), PC_98_RESOLUTION * 4);
    afterImage->set(afterPixbuf);
}

void MainWindow::OnChangeHueDither()
{
    conv->SetHueDitherFactor(huedither->get_value());
    memcpy(afterPixbuf->get_pixels(), conv->ReprocessGrabbedFrame(), PC_98_RESOLUTION * 4);
    afterImage->set(afterPixbuf);
}

void MainWindow::OnChangeUVBias()
{
    conv->SetUVBias(uvbias->get_value());
    memcpy(afterPixbuf->get_pixels(), conv->ReprocessGrabbedFrame(), PC_98_RESOLUTION * 4);
    afterImage->set(afterPixbuf);
}
