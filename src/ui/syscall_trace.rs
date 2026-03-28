use gtk::prelude::*;
use gtk4 as gtk;
use vte4 as vte;

#[derive(Clone)]
pub struct SyscallTraceView {
    pub root: gtk::Box,
    trace: gtk::TextView,
}

impl SyscallTraceView {
    pub fn new() -> Self {
        let root = gtk::Box::new(gtk::Orientation::Vertical, 0);
        let title = gtk::Label::new(Some("Trace"));
        let notebook = gtk::Notebook::new();
        let trace_scroller = gtk::ScrolledWindow::new();
        let trace = gtk::TextView::new();
        let terminal = vte::Terminal::new();

        root.add_css_class("panel");
        title.add_css_class("panel-header");
        title.set_xalign(0.0);

        trace.set_editable(false);
        trace.set_cursor_visible(false);
        trace.set_monospace(true);
        trace.add_css_class("mono");
        trace_scroller.set_vexpand(true);
        trace_scroller.set_child(Some(&trace));
        terminal.set_hexpand(true);
        terminal.set_vexpand(true);

        notebook.append_page(&trace_scroller, Some(&gtk::Label::new(Some("Trace"))));
        notebook.append_page(&terminal, Some(&gtk::Label::new(Some("Terminal"))));
        notebook.set_vexpand(true);

        root.append(&title);
        root.append(&notebook);

        Self { root, trace }
    }

    pub fn set_text(&self, text: &str) {
        self.trace.buffer().set_text(text);
    }
}
