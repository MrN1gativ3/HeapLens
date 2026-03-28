use gtk::prelude::*;
use gtk4 as gtk;

use crate::ui::tabs::heap_tab::HeapTabView;
use crate::ui::tabs::info_tab::InfoTabView;
use crate::ui::tabs::proc_tab::ProcTabView;
use crate::ui::tabs::regs_tab::RegsTabView;
use crate::ui::tabs::stack_tab::StackTabView;

#[derive(Clone)]
pub struct RightPanelView {
    pub root: gtk::Notebook,
    pub heap: HeapTabView,
    pub trace: StackTabView,
    pub regs: RegsTabView,
    pub proc_tab: ProcTabView,
    pub info: InfoTabView,
}

impl RightPanelView {
    pub fn new() -> Self {
        let root = gtk::Notebook::new();
        let heap = HeapTabView::new();
        let trace = StackTabView::new();
        let regs = RegsTabView::new();
        let proc_tab = ProcTabView::new();
        let info = InfoTabView::new();

        root.add_css_class("right-panel-tabs");
        root.set_size_request(228, -1);
        root.append_page(&heap.root, Some(&tab_label("HEAP")));
        root.append_page(&trace.root, Some(&tab_label("TRACE")));
        root.append_page(&regs.root, Some(&tab_label("REGS")));
        root.append_page(&proc_tab.root, Some(&tab_label("PROC")));
        root.append_page(&info.root, Some(&tab_label("INFO")));
        root.set_current_page(Some(4));

        Self {
            root,
            heap,
            trace,
            regs,
            proc_tab,
            info,
        }
    }
}

fn tab_label(text: &str) -> gtk::Label {
    gtk::Label::new(Some(text))
}
