use gtk::prelude::*;
use gtk4 as gtk;

#[derive(Clone)]
pub struct StatusbarView {
    pub root: gtk::Box,
    left: gtk::Label,
    center: gtk::Label,
    right: gtk::Label,
}

impl StatusbarView {
    pub fn new() -> Self {
        let root = gtk::Box::new(gtk::Orientation::Horizontal, 8);
        let left = gtk::Label::new(None);
        let center = gtk::Label::new(None);
        let right = gtk::Label::new(None);
        let left_box = gtk::Box::new(gtk::Orientation::Horizontal, 0);
        let center_box = gtk::Box::new(gtk::Orientation::Horizontal, 0);
        let right_box = gtk::Box::new(gtk::Orientation::Horizontal, 0);

        root.add_css_class("statusbar");
        root.set_size_request(-1, 22);
        left.add_css_class("status-left");
        center.add_css_class("status-center");
        right.add_css_class("status-right");
        left.set_xalign(0.0);
        center.set_xalign(0.5);
        right.set_xalign(1.0);
        left_box.set_hexpand(false);
        center_box.set_hexpand(true);
        right_box.set_hexpand(false);
        center_box.set_halign(gtk::Align::Center);
        right_box.set_halign(gtk::Align::End);
        left_box.append(&left);
        center_box.append(&center);
        right_box.append(&right);
        root.append(&left_box);
        root.append(&center_box);
        root.append(&right_box);

        let view = Self {
            root,
            left,
            center,
            right,
        };
        view.set_sections("IDLE", "ready", "heap unavailable");
        view
    }

    pub fn set_sections(&self, left: &str, center: &str, right: &str) {
        self.left.set_text(left);
        self.center.set_text(center);
        self.right.set_text(right);
    }
}
