use gtk::prelude::*;
use gtk4 as gtk;

#[derive(Clone)]
pub struct MenuBarView {
    pub root: gtk::Box,
}

impl MenuBarView {
    pub fn new() -> Self {
        let root = gtk::Box::new(gtk::Orientation::Horizontal, 0);

        root.add_css_class("menubar");
        root.set_size_request(-1, 22);

        for item in ["File", "View", "Debug", "Technique", "glibc", "Windows", "Help"] {
            let label = gtk::Label::new(Some(item));
            label.add_css_class("menu-item");
            label.set_xalign(0.0);
            root.append(&label);
        }

        Self { root }
    }
}
