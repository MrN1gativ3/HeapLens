use gtk::prelude::*;
use gtk4 as gtk;

use crate::app::HeapChunkView;

#[derive(Clone)]
pub struct HeapTabView {
    pub root: gtk::Box,
    title: gtk::Label,
    summary: gtk::Label,
    content: gtk::Box,
}

impl HeapTabView {
    pub fn new() -> Self {
        let root = gtk::Box::new(gtk::Orientation::Vertical, 0);
        let header = gtk::Box::new(gtk::Orientation::Horizontal, 0);
        let title = gtk::Label::new(Some("heap"));
        let spacer = gtk::Box::new(gtk::Orientation::Horizontal, 0);
        let summary = gtk::Label::new(Some("waiting"));
        let scroller = gtk::ScrolledWindow::new();
        let content = gtk::Box::new(gtk::Orientation::Vertical, 2);

        root.add_css_class("heap-tab");
        root.set_vexpand(true);
        header.add_css_class("heap-heading");
        title.add_css_class("heap-title");
        summary.add_css_class("heap-summary");
        title.set_xalign(0.0);
        summary.set_xalign(1.0);
        spacer.set_hexpand(true);
        header.append(&title);
        header.append(&spacer);
        header.append(&summary);

        scroller.set_hexpand(true);
        scroller.set_vexpand(true);
        scroller.set_propagate_natural_height(false);
        scroller.set_propagate_natural_width(false);
        scroller.set_child(Some(&content));

        root.append(&header);
        root.append(&scroller);

        let view = Self {
            root,
            title,
            summary,
            content,
        };
        view.clear();
        view
    }

    pub fn clear(&self) {
        self.title.set_text("heap");
        self.summary.set_text("waiting");
        clear_box(&self.content);
        let row = gtk::Label::new(Some("No heap snapshot loaded."));
        row.add_css_class("heap-meta");
        row.set_xalign(0.0);
        self.content.append(&row);
    }

    pub fn set_chunks(&self, summary: &str, chunks: &[HeapChunkView]) {
        self.title.set_text("main_arena");
        self.summary.set_text(summary);
        clear_box(&self.content);

        if chunks.is_empty() {
            let row = gtk::Label::new(Some("No decoded chunks available."));
            row.add_css_class("heap-meta");
            row.set_xalign(0.0);
            self.content.append(&row);
            return;
        }

        for chunk in chunks.iter().take(10) {
            let title = gtk::Label::new(Some(chunk.title.as_str()));
            title.add_css_class("heap-item");
            title.add_css_class(heap_class(&chunk.kind));
            title.set_xalign(0.0);
            self.content.append(&title);

            let detail = gtk::Label::new(Some(chunk.detail.as_str()));
            detail.add_css_class("heap-meta");
            detail.set_xalign(0.0);
            self.content.append(&detail);

            if !chunk.validation.is_empty() {
                let validation = gtk::Label::new(Some(chunk.validation.as_str()));
                validation.add_css_class("heap-alert");
                validation.set_xalign(0.0);
                self.content.append(&validation);
            }
        }
    }
}

fn heap_class(kind: &str) -> &'static str {
    match kind {
        "chunk-tcache" => "chunk-tcache",
        "chunk-fastbin" => "chunk-fastbin",
        "chunk-corrupt" => "chunk-corrupt",
        "chunk-top" => "chunk-top",
        "chunk-free" => "chunk-free",
        _ => "chunk-alloc",
    }
}

fn clear_box(container: &gtk::Box) {
    while let Some(child) = container.first_child() {
        container.remove(&child);
    }
}
