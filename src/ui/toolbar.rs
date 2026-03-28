use gtk::prelude::*;
use gtk4 as gtk;

use crate::app::MitigationBadge;

#[derive(Clone)]
pub struct ToolbarView {
    pub root: gtk::Box,
    pub restart_button: gtk::Button,
    pub run_button: gtk::Button,
    pub pause_button: gtk::Button,
    pub step_into_button: gtk::Button,
    pub step_over_button: gtk::Button,
    pub step_out_button: gtk::Button,
    pub run_to_cursor_button: gtk::Button,
    pub stop_button: gtk::Button,
    technique_button: gtk::Button,
    glibc_button: gtk::Button,
    badges_box: gtk::Box,
}

impl ToolbarView {
    pub fn new() -> Self {
        let root = gtk::Box::new(gtk::Orientation::Horizontal, 2);
        let left = gtk::Box::new(gtk::Orientation::Horizontal, 2);
        let right = gtk::Box::new(gtk::Orientation::Horizontal, 6);
        let spacer = gtk::Box::new(gtk::Orientation::Horizontal, 0);
        let open_button = icon_button("📂", "Open");
        let save_button = icon_button("💾", "Save");
        let restart_button = icon_button("⟲", "Restart");
        let run_button = icon_button("▶", "Run");
        let pause_button = icon_button("⏸", "Pause");
        let step_into_button = icon_button("↓", "Step Into");
        let step_over_button = icon_button("→", "Step Over");
        let step_out_button = icon_button("↑", "Step Out");
        let run_to_cursor_button = icon_button("⏭", "Run To Cursor");
        let stop_button = icon_button("■", "Stop");
        let technique_label = caption("TECHNIQUE:");
        let glibc_label = caption("GLIBC:");
        let technique_button = dropdown_button("Tcache Poison          ▾", 160);
        let glibc_button = dropdown_button("2.35 ▾", 64);
        let badges_box = gtk::Box::new(gtk::Orientation::Horizontal, 0);

        root.add_css_class("toolbar");
        root.set_size_request(-1, 36);
        spacer.set_hexpand(true);
        run_button.add_css_class("run-active");
        stop_button.add_css_class("stop-active");

        left.append(&open_button);
        left.append(&save_button);
        left.append(&separator());
        left.append(&restart_button);
        left.append(&run_button);
        left.append(&pause_button);
        left.append(&step_into_button);
        left.append(&step_over_button);
        left.append(&step_out_button);
        left.append(&run_to_cursor_button);
        left.append(&separator());
        left.append(&stop_button);

        right.append(&technique_label);
        right.append(&technique_button);
        right.append(&glibc_label);
        right.append(&glibc_button);
        right.append(&separator());
        right.append(&badges_box);

        root.append(&left);
        root.append(&spacer);
        root.append(&right);

        Self {
            root,
            restart_button,
            run_button,
            pause_button,
            step_into_button,
            step_over_button,
            step_out_button,
            run_to_cursor_button,
            stop_button,
            technique_button,
            glibc_button,
            badges_box,
        }
    }

    pub fn set_technique_label(&self, label: &str) {
        self.technique_button
            .set_label(&format!("{:<20} ▾", compact_label(label, 20)));
    }

    pub fn set_glibc_label(&self, label: &str) {
        self.glibc_button.set_label(&format!("{} ▾", label));
    }

    pub fn connect_glibc_clicked<F>(&self, callback: F)
    where
        F: Fn() + 'static,
    {
        let button = self.glibc_button.clone();
        button.connect_clicked(move |_| callback());
    }

    pub fn set_mitigations(&self, badges: &[MitigationBadge]) {
        while let Some(child) = self.badges_box.first_child() {
            self.badges_box.remove(&child);
        }

        for badge in badges {
            let label = gtk::Label::new(Some(badge.name));
            label.add_css_class("badge");
            label.add_css_class(if badge.enabled { "badge-on" } else { "badge-off" });
            label.set_tooltip_text(Some(badge.detail));
            self.badges_box.append(&label);
        }
    }
}

fn icon_button(text: &str, tooltip: &str) -> gtk::Button {
    let button = gtk::Button::with_label(text);
    button.add_css_class("toolbar-icon-btn");
    button.set_tooltip_text(Some(tooltip));
    button
}

fn dropdown_button(text: &str, width: i32) -> gtk::Button {
    let button = gtk::Button::with_label(text);
    button.add_css_class("dropdown-btn");
    button.set_size_request(width, 22);
    button
}

fn caption(text: &str) -> gtk::Label {
    let label = gtk::Label::new(Some(text));
    label.add_css_class("toolbar-caption");
    label
}

fn separator() -> gtk::Separator {
    let separator = gtk::Separator::new(gtk::Orientation::Vertical);
    separator.add_css_class("toolbar-separator");
    separator.set_margin_start(4);
    separator.set_margin_end(4);
    separator
}

fn compact_label(text: &str, width: usize) -> String {
    let mut value: String = text.chars().take(width).collect();
    while value.chars().count() < width {
        value.push(' ');
    }
    value
}
