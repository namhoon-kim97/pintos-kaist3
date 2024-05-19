/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/mmu.h"
#include "threads/palloc.h"
struct load_info {
    struct file *file;
    size_t page_read_bytes;
    size_t page_zero_bytes;
    off_t offset;
};
static bool file_backed_swap_in(struct page *page, void *kva);
static bool file_backed_swap_out(struct page *page);
static void file_backed_destroy(struct page *page);
static bool lazy_load_file(struct page *page, void *aux);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
    .swap_in = file_backed_swap_in,
    .swap_out = file_backed_swap_out,
    .destroy = file_backed_destroy,
    .type = VM_FILE,
};

/* The initializer of file vm */
void vm_file_init(void) {
}

/* Initialize the file backed page */
bool file_backed_initializer(struct page *page, enum vm_type type, void *kva) {
    /* Set up the handler */
    page->operations = &file_ops;

    struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in(struct page *page, void *kva) {
    struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out(struct page *page) {
    struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy(struct page *page) {
    struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap(void *addr, size_t length, int writable,
        struct file *file, off_t offset) {
    void *ret = addr;

    size_t read_bytes = length;
    size_t zero_bytes = length % PGSIZE;
    while (read_bytes > 0 || zero_bytes > 0) {
        /* Do calculate how to fill this page.
         * We will read PAGE_READ_BYTES bytes from FILE
         * and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        /* TODO: Set up aux to pass information to the lazy_load_segment. */
        struct load_info *info = malloc(sizeof *info);
        if (!info)
            return NULL;
        info->file = file;
        info->page_read_bytes = page_read_bytes;
        info->page_zero_bytes = page_zero_bytes;
        info->offset = offset;
        if (!vm_alloc_page_with_initializer(VM_ANON, addr, writable, lazy_load_file, info)) {
            return NULL;
        }

        /* Advance. */
        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        addr += PGSIZE;
        offset += page_read_bytes;
    }
    return ret;
}

/* Do the munmap */
void do_munmap(void *addr) {
}

static bool
lazy_load_file(struct page *page, void *aux) {
    /* TODO: Load the segment from the file */
    /* TODO: This called when the first page fault occurs on address VA. */
    /* TODO: VA is available when calling this function. */
    struct load_info *info = (struct load_info *)aux;
    uint8_t *kpage = page->frame->kva;
    if (kpage == NULL)
        return false;
    file_seek(info->file, info->offset);
    /* Load this page. */
    if (file_read(info->file, kpage, info->page_read_bytes) != (int)info->page_read_bytes)
        return false;

    return true;
}