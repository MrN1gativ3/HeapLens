#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "${SCRIPT_DIR}/.." && pwd)
TOOL_DIR="${GLIBC_ALL_IN_ONE_DIR:-${REPO_ROOT}/.cache/glibc-all-in-one}"
TARGET_DIR="${GLIBC_TARGET_DIR:-${REPO_ROOT}/glibc}"
VERSIONS=(2.23 2.27 2.29 2.31 2.32 2.34 2.35 2.38 2.39)

find_package() {
    local version="$1"
    local list_name
    local candidate

    for list_name in list old_list; do
        if [[ ! -f "${TOOL_DIR}/${list_name}" ]]; then
            continue
        fi
        candidate=$(
            grep -E "^${version}-.*_amd64$" "${TOOL_DIR}/${list_name}" \
                | sort -V \
                | tail -n 1 \
                || true
        )
        if [[ -n "${candidate}" ]]; then
            printf '%s:%s\n' "${list_name}" "${candidate}"
            return 0
        fi
    done

    return 1
}

copy_runtime_tree() {
    local source_dir="$1"
    local dest_dir="$2"
    local path
    local loader_name
    local libc_name

    mkdir -p "${dest_dir}"
    shopt -s dotglob nullglob
    for path in "${source_dir}"/*; do
        case "$(basename "${path}")" in
            .debug)
                continue
                ;;
        esac
        cp -a "${path}" "${dest_dir}/"
    done
    shopt -u dotglob nullglob

    loader_name=$(
        find "${dest_dir}" -maxdepth 1 \( -type f -o -type l \) \
            \( -name 'ld-*.so' -o -name 'ld-linux-*.so*' \) \
            | sort \
            | head -n 1 \
            | xargs -r basename
    )
    libc_name=$(find "${dest_dir}" -maxdepth 1 \( -type f -o -type l \) -name 'libc-*.so' | sort | head -n 1 | xargs -r basename)

    if [[ -n "${loader_name}" ]]; then
        ln -sfn "${loader_name}" "${dest_dir}/ld.so"
    fi
    if [[ ! -e "${dest_dir}/libc.so.6" && -n "${libc_name}" ]]; then
        ln -sfn "${libc_name}" "${dest_dir}/libc.so.6"
    fi
}

if [[ ! -d "${TOOL_DIR}/.git" ]]; then
    mkdir -p "$(dirname "${TOOL_DIR}")"
    git clone https://github.com/matrix1001/glibc-all-in-one.git "${TOOL_DIR}"
else
    git -C "${TOOL_DIR}" pull --ff-only
fi

(
    cd "${TOOL_DIR}"
    ./update_list
)

mkdir -p "${TARGET_DIR}"

for version in "${VERSIONS[@]}"; do
    package_record=$(find_package "${version}") || {
        printf 'No amd64 package found for glibc %s\n' "${version}" >&2
        exit 1
    }
    list_name=${package_record%%:*}
    package_name=${package_record#*:}
    downloader="download"
    if [[ "${list_name}" == "old_list" ]]; then
        downloader="download_old"
    fi

    if [[ ! -d "${TOOL_DIR}/libs/${package_name}" ]]; then
        (
            cd "${TOOL_DIR}"
            "./${downloader}" "${package_name}"
        )
    fi

    copy_runtime_tree "${TOOL_DIR}/libs/${package_name}" "${TARGET_DIR}/${version}/lib"
    printf 'Installed glibc %s from %s\n' "${version}" "${package_name}"
done

printf 'Rebuild demos with `make -B demos` to relink them against the installed runtimes.\n'
