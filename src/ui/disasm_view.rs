use gtk::prelude::*;
use gtk4 as gtk;

use crate::app::DisasmRow;

#[derive(Clone)]
pub struct DisasmView {
    pub root: gtk::Box,
    list: gtk::ListBox,
}

impl DisasmView {
    pub fn new() -> Self {
        let root = gtk::Box::new(gtk::Orientation::Vertical, 0);
        let header = gtk::Box::new(gtk::Orientation::Horizontal, 8);
        let title = gtk::Label::new(Some("CPU"));
        let spacer = gtk::Box::new(gtk::Orientation::Horizontal, 0);
        let follow = gtk::Entry::new();
        let scroller = gtk::ScrolledWindow::new();
        let list = gtk::ListBox::new();

        root.add_css_class("cpu-panel");
        title.add_css_class("cpu-label");
        header.add_css_class("cpu-header");
        header.set_size_request(-1, 18);
        spacer.set_hexpand(true);
        follow.add_css_class("follow-entry");
        follow.set_placeholder_text(Some("Follow expr"));
        follow.set_width_chars(20);
        list.add_css_class("disasm-area");
        list.set_selection_mode(gtk::SelectionMode::None);
        scroller.set_vexpand(true);
        scroller.set_child(Some(&list));

        header.append(&title);
        header.append(&spacer);
        header.append(&follow);
        root.append(&header);
        root.append(&scroller);

        let view = Self { root, list };
        view.clear();
        view
    }

    pub fn clear(&self) {
        self.set_rows(&[]);
    }

    pub fn set_rows(&self, rows: &[DisasmRow]) {
        clear_list(&self.list);

        if rows.is_empty() {
            let row = gtk::ListBoxRow::new();
            let label = gtk::Label::new(Some("No disassembly loaded."));
            row.add_css_class("disasm-row-even");
            label.add_css_class("comment");
            label.set_xalign(0.0);
            row.set_child(Some(&label));
            self.list.append(&row);
            return;
        }

        for (index, row_data) in rows.iter().enumerate() {
            let row = gtk::ListBoxRow::new();
            let content = gtk::Box::new(gtk::Orientation::Horizontal, 0);
            let marker = gtk::Label::new(Some(if row_data.breakpoint {
                "■"
            } else if row_data.current_ip {
                "▶"
            } else {
                ""
            }));
            let address = gtk::Label::new(Some(row_data.address.as_str()));
            let bytes = gtk::Label::new(Some(row_data.bytes.as_str()));
            let disasm = gtk::Label::new(Some(row_data.mnemonic.as_str()));
            let comment = gtk::Label::new(Some(row_data.comment.as_str()));

            row.add_css_class(if index % 2 == 0 {
                "disasm-row-even"
            } else {
                "disasm-row-odd"
            });
            if row_data.current_ip {
                row.add_css_class("current-ip");
            }
            if row_data.breakpoint {
                row.add_css_class("breakpoint");
            }

            marker.set_width_request(12);
            address.set_width_request(130);
            bytes.set_width_request(90);
            disasm.set_width_request(340);
            disasm.set_hexpand(true);
            comment.set_hexpand(true);

            marker.set_xalign(0.5);
            address.set_xalign(0.0);
            bytes.set_xalign(0.0);
            disasm.set_xalign(0.0);
            comment.set_xalign(0.0);

            marker.add_css_class(if row_data.breakpoint { "bp-break" } else { "bp-arrow" });
            address.add_css_class("addr");
            bytes.add_css_class("bytes");
            disasm.add_css_class("mnem");
            comment.add_css_class("comment");

            if row_data.current_ip {
                address.add_css_class("current-ip-text");
                bytes.add_css_class("current-ip-text");
                disasm.add_css_class("current-ip-text");
            }

            content.append(&marker);
            content.append(&address);
            content.append(&bytes);
            content.append(&disasm);
            content.append(&comment);
            row.set_child(Some(&content));
            self.list.append(&row);
        }
    }
}

fn clear_list(list: &gtk::ListBox) {
    while let Some(child) = list.first_child() {
        list.remove(&child);
    }
}
