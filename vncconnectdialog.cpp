/* This file is part of gsshvnc.
 *
 * gsshvnc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * gsshvnc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gsshvnc.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "vncconnectdialog.h"
#include "vncdisplaymm.h"

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/entry.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/comboboxtext.h>

Vnc::ConnectDialog::ConnectDialog(Gtk::Window &parent)
    : Gtk::Dialog("Connect", parent, Gtk::DIALOG_MODAL | Gtk::DIALOG_DESTROY_WITH_PARENT)
{
    add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    add_button("Co_nnect", Gtk::RESPONSE_OK);
    set_default_response(Gtk::RESPONSE_OK);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5));
    box->set_border_width(8);

    auto label = Gtk::manage(new Gtk::Label);
    label->set_markup("<b>Connection Details</b>");
    label->set_alignment(Gtk::ALIGN_START, Gtk::ALIGN_CENTER);

    box->add(*label);

    auto linebox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
    linebox->set_margin_left(15);
    label = Gtk::manage(new Gtk::Label("_Host:", true));
    m_host = Gtk::manage(new Gtk::Entry);
    m_host->set_alignment(Gtk::ALIGN_FILL);
    m_host->set_placeholder_text("hostname[:display]");
    m_host->set_activates_default(true);

    linebox->pack_start(*label, Gtk::PACK_SHRINK);
    linebox->pack_start(*m_host);
    box->add(*linebox);

    linebox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
    linebox->set_margin_left(15);
    label = Gtk::manage(new Gtk::Label("SSH _Tunnel:", true));
    m_ssh_host = Gtk::manage(new Gtk::Entry);
    m_ssh_host->set_alignment(Gtk::ALIGN_FILL);
    m_ssh_host->set_placeholder_text("[user@]hostname[:port]");
    m_ssh_host->set_activates_default(true);

    linebox->pack_start(*label, Gtk::PACK_SHRINK);
    linebox->pack_start(*m_ssh_host);
    box->add(*linebox);

    label = Gtk::manage(new Gtk::Label);
    label->set_markup("<b>VNC Options</b>");
    label->set_alignment(Gtk::ALIGN_START, Gtk::ALIGN_CENTER);
    label->set_margin_top(10);

    box->add(*label);

    m_lossy_compression = Gtk::manage(new Gtk::CheckButton("Use Lossy (_JPEG) Compression", true));
    m_lossy_compression->set_margin_left(15);
    m_lossy_compression->set_active(true);

    box->add(*m_lossy_compression);

    linebox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
    linebox->set_margin_left(15);
    label = Gtk::manage(new Gtk::Label("_Color Depth:", true));
    label->set_alignment(Gtk::ALIGN_FILL, Gtk::ALIGN_CENTER);
    m_color_depth = Gtk::manage(new Gtk::ComboBoxText);
    m_color_depth->append(std::to_string(VNC_DISPLAY_DEPTH_COLOR_DEFAULT), "Use Server's Setting");
    m_color_depth->append(std::to_string(VNC_DISPLAY_DEPTH_COLOR_FULL), "True Color (24 bits)");
    m_color_depth->append(std::to_string(VNC_DISPLAY_DEPTH_COLOR_MEDIUM), "High Color (16 bits)");
    m_color_depth->append(std::to_string(VNC_DISPLAY_DEPTH_COLOR_LOW), "Low Color (8 bits)");
    m_color_depth->append(std::to_string(VNC_DISPLAY_DEPTH_COLOR_ULTRA_LOW), "Ultra Low Color (3 bits)");
    m_color_depth->set_active(0);

    linebox->pack_start(*label, Gtk::PACK_SHRINK);
    linebox->pack_start(*m_color_depth, Gtk::PACK_SHRINK);
    box->add(*linebox);

    auto vbox = get_child();
    dynamic_cast<Gtk::Container *>(vbox)->add(*box);
}

bool Vnc::ConnectDialog::configure(Vnc::DisplayWindow &vnc, SshTunnel &tunnel)
{
    vnc.set_shared_flag(true);
    vnc.set_depth((VncDisplayDepthColor)std::stoi(m_color_depth->get_active_id()));
    vnc.set_lossy_encoding(m_lossy_compression->get_active());

    Glib::ustring hostname = m_host->get_text();
    Glib::ustring port;

    auto ppos = hostname.find(':');
    if (ppos != Glib::ustring::npos) {
        port = std::to_string(5900 + std::stoi(hostname.substr(ppos + 1)));
        hostname.resize(ppos);
    } else {
        port = "5900";
    }

    if (hostname.empty())
        hostname = "127.0.0.1";

    Glib::ustring ssh_string = m_ssh_host->get_text();
    if (!ssh_string.empty()) {
        if (!tunnel.connect(ssh_string))
            return false;
        guint16 local_port = tunnel.forward_port(hostname, std::stoi(port));
        if (local_port == 0)
            return false;
        hostname = "127.0.0.1";
        port = std::to_string(local_port);
    }

    if (!vnc.open_host(hostname, port))
        return false;

    vnc.set_keyboard_grab(true);
    vnc.set_pointer_grab(true);
    vnc.set_pointer_local(true);
    vnc.set_grab_keyboard(true);

    if (!vnc.is_composited())
        vnc.set_scaling(true);

    return true;
}