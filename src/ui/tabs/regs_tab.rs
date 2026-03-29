use gtk::prelude::*;
use gtk4 as gtk;

use crate::app::{FlagState, RegisterRow};

#[derive(Clone)]
pub struct RegsTabView {
    pub root: gtk::Box,
    rip_addr: gtk::Label,
    rip_symbol: gtk::Label,
    flags_row: gtk::Box,
    list: gtk::ListBox,
}

impl RegsTabView {
    pub fn new() -> Self {
        let root = gtk::Box::new(gtk::Orientation::Vertical, 0);
        let rip_bar = gtk::Box::new(gtk::Orientation::Horizontal, 8);
        let rip_label = gtk::Label::new(Some("RIP"));
        let rip_addr = gtk::Label::new(Some("0x0000000000000000"));
        let rip_symbol = gtk::Label::new(Some("<waiting>"));
        let flags_row = gtk::Box::new(gtk::Orientation::Horizontal, 2);
        let scroller = gtk::ScrolledWindow::new();
        let list = gtk::ListBox::new();

        root.add_css_class("regs-root");
        root.set_vexpand(true);
        rip_bar.add_css_class("rip-bar");
        rip_label.add_css_class("rip-label");
        rip_addr.add_css_class("rip-addr");
        rip_symbol.add_css_class("rip-symbol");
        rip_label.set_xalign(0.0);
        rip_addr.set_xalign(0.0);
        rip_symbol.set_xalign(0.0);
        flags_row.add_css_class("flags-row");
        scroller.set_hexpand(true);
        scroller.set_vexpand(true);
        scroller.set_propagate_natural_height(false);
        scroller.set_propagate_natural_width(false);
        list.add_css_class("regs-list");
        list.set_selection_mode(gtk::SelectionMode::None);
        scroller.set_child(Some(&list));

        rip_bar.append(&rip_label);
        rip_bar.append(&rip_addr);
        rip_bar.append(&rip_symbol);
        root.append(&rip_bar);
        root.append(&flags_row);
        root.append(&scroller);

        let view = Self {
            root,
            rip_addr,
            rip_symbol,
            flags_row,
            list,
        };
        view.clear();
        view
    }

    pub fn clear(&self) {
        self.rip_addr.set_text("0x0000000000000000");
        self.rip_symbol.set_text("<waiting>");
        clear_box(&self.flags_row);
        clear_list(&self.list);
    }

    pub fn set_snapshot(&self, rip_address: &str, rip_symbol: &str, flags: &[FlagState], rows: &[RegisterRow]) {
        self.rip_addr.set_text(rip_address);
        self.rip_symbol.set_text(rip_symbol);

        clear_box(&self.flags_row);
        for flag in flags.iter().take(8) {
            let pill = gtk::Label::new(Some(&format!("{}:{}", flag.name, if flag.set { 1 } else { 0 })));
            pill.add_css_class("flag-pill");
            pill.add_css_class(if flag.set { "flag-set" } else { "flag-clear" });
            self.flags_row.append(&pill);
        }

        clear_list(&self.list);
        for (index, reg) in rows.iter().take(13).enumerate() {
            let row = gtk::ListBoxRow::new();
            let row_box = gtk::Box::new(gtk::Orientation::Vertical, 0);
            let line1 = gtk::Box::new(gtk::Orientation::Horizontal, 8);
            let line2 = gtk::Label::new(Some(reg.decimal.as_str()));
            let line3 = gtk::Label::new(Some(reg.ascii.as_str()));
            let name = gtk::Label::new(Some(reg.name));
            let value = gtk::Label::new(Some(reg.hex.as_str()));
            let spacer = gtk::Box::new(gtk::Orientation::Horizontal, 0);

            row.add_css_class(if index % 2 == 0 { "reg-row-even" } else { "reg-row-odd" });
            row_box.add_css_class("reg-row-box");
            name.add_css_class("reg-label");
            value.add_css_class("reg-hex");
            if reg.changed {
                value.add_css_class("reg-changed");
            }
            line2.add_css_class("reg-dec");
            line3.add_css_class("reg-ascii");
            name.set_xalign(0.0);
            value.set_xalign(1.0);
            line2.set_xalign(0.0);
            line3.set_xalign(0.0);
            spacer.set_hexpand(true);

            line1.append(&name);
            line1.append(&spacer);
            line1.append(&value);
            row_box.append(&line1);
            row_box.append(&line2);
            row_box.append(&line3);
            row.set_child(Some(&row_box));
            self.list.append(&row);
        }
    }
}

fn clear_box(container: &gtk::Box) {
    while let Some(child) = container.first_child() {
        container.remove(&child);
    }
}

fn clear_list(list: &gtk::ListBox) {
    while let Some(child) = list.first_child() {
        list.remove(&child);
    }
}
