use std::cell::RefCell;
use std::rc::Rc;

use gtk::prelude::*;
use gtk4 as gtk;

use crate::app::Technique;

#[derive(Clone)]
pub struct TechniqueListView {
    pub root: gtk::Box,
    list: gtk::ListBox,
    rows: Rc<RefCell<Vec<RowMeta>>>,
}

#[derive(Clone)]
struct RowMeta {
    label: String,
    actual_index: Option<usize>,
}

enum VisibleEntry {
    Category(&'static str),
    Technique(&'static str, Option<&'static str>),
}

impl TechniqueListView {
    pub fn new(techniques: &[Technique]) -> Self {
        let root = gtk::Box::new(gtk::Orientation::Vertical, 0);
        let title = gtk::Label::new(Some("TECHNIQUES"));
        let scroller = gtk::ScrolledWindow::new();
        let list = gtk::ListBox::new();
        let rows = Rc::new(RefCell::new(Vec::new()));

        root.add_css_class("techniques-panel");
        title.add_css_class("panel-header");
        title.set_xalign(0.0);
        scroller.set_vexpand(true);
        list.add_css_class("technique-list");
        list.set_selection_mode(gtk::SelectionMode::Single);
        scroller.set_child(Some(&list));
        root.set_size_request(138, -1);
        root.append(&title);
        root.append(&scroller);

        let view = Self { root, list, rows };
        view.populate(techniques);
        view.select_label("tcache Poisoning");
        view
    }

    pub fn connect_selected<F>(&self, callback: F)
    where
        F: Fn(String, Option<usize>) + 'static,
    {
        let rows = self.rows.clone();
        self.list.connect_row_selected(move |_, row| {
            let Some(row) = row else {
                return;
            };
            let index = row.index();
            if index < 0 {
                return;
            }

            if let Some(meta) = rows.borrow().get(index as usize) {
                callback(meta.label.clone(), meta.actual_index);
            }
        });
    }

    pub fn select_label(&self, label: &str) {
        for (index, meta) in self.rows.borrow().iter().enumerate() {
            if meta.label == label {
                if let Some(row) = self.list.row_at_index(index as i32) {
                    self.list.select_row(Some(&row));
                }
                break;
            }
        }
    }

    fn populate(&self, techniques: &[Technique]) {
        while let Some(child) = self.list.first_child() {
            self.list.remove(&child);
        }
        self.rows.borrow_mut().clear();

        for entry in VISIBLE_TECHNIQUES {
            match entry {
                VisibleEntry::Category(text) => {
                    let row = gtk::ListBoxRow::new();
                    let label = gtk::Label::new(Some(text));
                    row.set_selectable(false);
                    row.set_activatable(false);
                    label.add_css_class("category-header");
                    label.set_xalign(0.0);
                    row.set_child(Some(&label));
                    self.list.append(&row);
                    self.rows.borrow_mut().push(RowMeta {
                        label: (*text).to_string(),
                        actual_index: None,
                    });
                }
                VisibleEntry::Technique(text, actual_id) => {
                    let row = gtk::ListBoxRow::new();
                    let label = gtk::Label::new(Some(text));
                    let actual_index = actual_id.and_then(|id| {
                        techniques
                            .iter()
                            .find(|item| item.id == id)
                            .map(|item| item.index)
                    });

                    label.add_css_class("technique-item");
                    label.set_xalign(0.0);
                    row.set_child(Some(&label));
                    self.list.append(&row);
                    self.rows.borrow_mut().push(RowMeta {
                        label: (*text).to_string(),
                        actual_index,
                    });
                }
            }
        }
    }
}

const VISIBLE_TECHNIQUES: &[VisibleEntry] = &[
    VisibleEntry::Category("— GLIBC BASICS —"),
    VisibleEntry::Technique("malloc/free inspect", Some("malloc_free_fundamentals")),
    VisibleEntry::Technique("Heap layout walk", Some("heap_layout_walkthrough")),
    VisibleEntry::Technique("Bin state explorer", Some("bin_state_explorer")),
    VisibleEntry::Category("— USE-AFTER-FREE —"),
    VisibleEntry::Technique("UAF Basic", Some("uaf_basic")),
    VisibleEntry::Technique("UAF Type Confusion", Some("uaf_type_confusion")),
    VisibleEntry::Technique("UAF to Arb Read", Some("uaf_to_arbitrary_read")),
    VisibleEntry::Technique("UAF to Arb Write", Some("uaf_to_arbitrary_write")),
    VisibleEntry::Technique("UAF to Code Exec", Some("uaf_to_code_execution")),
    VisibleEntry::Technique("UAF via tcache", Some("uaf_via_tcache")),
    VisibleEntry::Technique("UAF via fastbin", Some("uaf_via_fastbin")),
    VisibleEntry::Category("— HEAP OVERFLOW —"),
    VisibleEntry::Technique("Classic Overflow", Some("classic_heap_overflow")),
    VisibleEntry::Technique("Off-by-One", Some("off_by_one")),
    VisibleEntry::Technique("Off-by-Null", Some("off_by_null")),
    VisibleEntry::Technique("OBO into Size Field", Some("off_by_one_into_size_field")),
    VisibleEntry::Technique("Overflow → Func Ptr", Some("heap_overflow_into_function_pointer")),
    VisibleEntry::Category("— DOUBLE FREE —"),
    VisibleEntry::Technique("Classic DblFree", Some("classic_double_free")),
    VisibleEntry::Technique("Fastbin DblFree", Some("fastbin_double_free")),
    VisibleEntry::Technique("tcache DblFree <2.29", Some("tcache_double_free_pre_229")),
    VisibleEntry::Technique("tcache DblFree ≥2.29", Some("tcache_double_free_key_bypass")),
    VisibleEntry::Technique("DblFree SafeLink bypass", Some("double_free_safe_linking_bypass")),
    VisibleEntry::Technique("DblFree + Arb Alloc", Some("double_free_to_arbitrary_allocation")),
    VisibleEntry::Category("— TCACHE —"),
    VisibleEntry::Technique("tcache Poisoning", Some("tcache_poisoning")),
    VisibleEntry::Technique("tcache Dup", Some("tcache_dup")),
    VisibleEntry::Technique("Metadata Corrupt", Some("tcache_metadata_corruption")),
    VisibleEntry::Technique("Perthread Overwrite", Some("tcache_perthread_struct_overwrite")),
    VisibleEntry::Technique("Stash Unlink", Some("tcache_stashing_unlink_attack")),
    VisibleEntry::Technique("Stash Unlink+", Some("tcache_stashing_unlink_attack_plus")),
    VisibleEntry::Technique("House of Spirit (tc)", Some("tcache_house_of_spirit")),
    VisibleEntry::Category("— FASTBIN —"),
    VisibleEntry::Technique("Fastbin Dup", Some("fastbin_dup")),
    VisibleEntry::Technique("Fastbin + Stack", Some("fastbin_dup_into_stack")),
    VisibleEntry::Technique("Dup Consolidate", Some("fastbin_dup_consolidate")),
    VisibleEntry::Technique("Fastbin+Other Bin", Some("fastbin_into_other_bin")),
    VisibleEntry::Technique("House of Spirit", Some("house_of_spirit")),
    VisibleEntry::Technique("Fastbin Corrupt", Some("fastbin_corruption")),
    VisibleEntry::Category("— UNSORTED BIN —"),
    VisibleEntry::Technique("Unsorted Bin Attack", Some("unsorted_bin_attack")),
    VisibleEntry::Technique("Unsorted Bin Leak", Some("unsorted_bin_leak")),
    VisibleEntry::Technique("Unsorted+Stack", Some("unsorted_bin_into_stack")),
    VisibleEntry::Technique("UB Corruption", Some("unsorted_bin_corruption")),
];
