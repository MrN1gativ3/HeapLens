use gtk::prelude::*;
use gtk4 as gtk;

#[derive(Clone)]
pub struct ProcTabView {
    pub root: gtk::Box,
    content: gtk::Box,
}

impl ProcTabView {
    pub fn new() -> Self {
        let root = gtk::Box::new(gtk::Orientation::Vertical, 0);
        let scroller = gtk::ScrolledWindow::new();
        let content = gtk::Box::new(gtk::Orientation::Vertical, 0);

        root.add_css_class("proc-tab");
        scroller.set_vexpand(true);
        scroller.set_child(Some(&content));
        root.append(&scroller);

        let view = Self { root, content };
        view.set_text("Target: waiting");
        view
    }

    pub fn set_text(&self, text: &str) {
        clear_box(&self.content);

        for line in text.lines() {
            let row = gtk::Box::new(gtk::Orientation::Horizontal, 8);
            let name = gtk::Label::new(None);
            let detail = gtk::Label::new(None);
            let spacer = gtk::Box::new(gtk::Orientation::Horizontal, 0);

            row.add_css_class("proc-row");
            name.add_css_class("proc-key");
            detail.add_css_class("proc-value");
            name.set_xalign(0.0);
            detail.set_xalign(1.0);
            spacer.set_hexpand(true);

            if let Some((lhs, rhs)) = line.split_once(':') {
                name.set_text(&format!("{}:", lhs.trim()));
                detail.set_text(rhs.trim());
            } else {
                name.set_text(line.trim());
                detail.set_text("");
            }

            row.append(&name);
            row.append(&spacer);
            row.append(&detail);
            self.content.append(&row);
        }
    }
}

fn clear_box(container: &gtk::Box) {
    while let Some(child) = container.first_child() {
        container.remove(&child);
    }
}
