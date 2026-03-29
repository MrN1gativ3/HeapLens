use gtk::prelude::*;
use gtk4 as gtk;

#[derive(Clone)]
pub struct StackTabView {
    pub root: gtk::Box,
    content: gtk::Box,
}

impl StackTabView {
    pub fn new() -> Self {
        let root = gtk::Box::new(gtk::Orientation::Vertical, 0);
        let scroller = gtk::ScrolledWindow::new();
        let content = gtk::Box::new(gtk::Orientation::Vertical, 0);

        root.add_css_class("trace-tab");
        root.set_vexpand(true);
        scroller.set_hexpand(true);
        scroller.set_vexpand(true);
        scroller.set_propagate_natural_height(false);
        scroller.set_propagate_natural_width(false);
        scroller.set_child(Some(&content));
        root.append(&scroller);

        let view = Self { root, content };
        view.set_lines(&[]);
        view
    }

    pub fn set_lines(&self, lines: &[String]) {
        clear_box(&self.content);

        let mut appended = false;
        for (index, text) in lines.iter().rev().take(20).collect::<Vec<_>>().into_iter().rev().enumerate() {
            let row = gtk::Label::new(Some(text.as_str()));
            row.add_css_class("trace-row");
            row.add_css_class(if index % 2 == 0 {
                "trace-row-even"
            } else {
                "trace-row-odd"
            });
            row.set_ellipsize(gtk::pango::EllipsizeMode::End);
            row.set_single_line_mode(true);
            row.set_xalign(0.0);
            self.content.append(&row);
            appended = true;
        }

        if !appended {
            let row = gtk::Label::new(Some("No trace lines yet."));
            row.add_css_class("trace-row");
            row.add_css_class("trace-row-even");
            row.set_ellipsize(gtk::pango::EllipsizeMode::End);
            row.set_single_line_mode(true);
            row.set_xalign(0.0);
            self.content.append(&row);
        }
    }
}

fn clear_box(container: &gtk::Box) {
    while let Some(child) = container.first_child() {
        container.remove(&child);
    }
}
