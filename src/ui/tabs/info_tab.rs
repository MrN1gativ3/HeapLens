use gtk::prelude::*;
use gtk4 as gtk;

#[derive(Clone)]
pub struct InfoTabView {
    pub root: gtk::ScrolledWindow,
    title: gtk::Label,
    subtitle: gtk::Label,
    body: gtk::Label,
    detail: gtk::Label,
    diagram: gtk::Label,
    alert: gtk::Label,
    logs: gtk::Label,
}

impl InfoTabView {
    pub fn new() -> Self {
        let root = gtk::ScrolledWindow::new();
        let content = gtk::Box::new(gtk::Orientation::Vertical, 10);
        let title = gtk::Label::new(Some("WAITING"));
        let subtitle = gtk::Label::new(Some("Select a technique"));
        let body = gtk::Label::new(Some("Technique notes will appear here."));
        let detail = gtk::Label::new(Some(""));
        let diagram = gtk::Label::new(Some(""));
        let alert = gtk::Label::new(Some(""));
        let logs = gtk::Label::new(Some(""));

        root.set_hexpand(true);
        root.set_vexpand(true);
        root.set_propagate_natural_height(false);
        root.set_propagate_natural_width(false);
        content.set_margin_top(10);
        content.set_margin_bottom(10);
        content.set_margin_start(10);
        content.set_margin_end(10);
        content.set_valign(gtk::Align::Start);

        title.add_css_class("info-title");
        subtitle.add_css_class("info-subtitle");
        body.add_css_class("info-copy");
        detail.add_css_class("info-step");
        diagram.add_css_class("info-diagram");
        alert.add_css_class("info-alert");
        logs.add_css_class("info-log");

        for label in [&title, &subtitle, &body, &detail, &diagram, &alert, &logs] {
            label.set_xalign(0.0);
            label.set_wrap(true);
        }

        content.append(&title);
        content.append(&subtitle);
        content.append(&body);
        content.append(&detail);
        content.append(&diagram);
        content.append(&alert);
        content.append(&logs);
        root.set_child(Some(&content));

        Self {
            root,
            title,
            subtitle,
            body,
            detail,
            diagram,
            alert,
            logs,
        }
    }

    pub fn set_content(
        &self,
        title: &str,
        subtitle: &str,
        body: &str,
        detail: &str,
        diagram: &str,
        alert: &str,
        logs: &str,
    ) {
        self.title.set_text(title);
        self.subtitle.set_text(subtitle);
        self.body.set_text(body);
        self.detail.set_text(detail);
        self.diagram.set_text(diagram);
        self.alert.set_text(alert);
        self.logs.set_text(logs);
    }
}
