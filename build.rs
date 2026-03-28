use std::env;
use std::path::PathBuf;

fn main() {
    let lib_dir = env::var("HEAPLENS_LIB_DIR").unwrap_or_else(|_| "build".to_string());
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap_or_else(|_| ".".to_string());
    let absolute_dir = PathBuf::from(&manifest_dir).join(&lib_dir);
    let lib_path = absolute_dir.join("libheaplens.so");

    println!("cargo:rerun-if-env-changed=HEAPLENS_LIB_DIR");
    println!("cargo:rerun-if-changed={}", lib_path.display());
    println!("cargo:rustc-link-search=native={}", absolute_dir.display());
    println!("cargo:rustc-link-lib=dylib=heaplens");
}
