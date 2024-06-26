/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/mmu.h"
#include "threads/palloc.h"

static bool file_backed_swap_in(struct page *page, void *kva);
static bool file_backed_swap_out(struct page *page);
static void file_backed_destroy(struct page *page);
static bool lazy_load_file(struct page *page, void *aux);

struct lock file_swap_lock;
extern struct lock frame_lock;
/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
    .swap_in = file_backed_swap_in,
    .swap_out = file_backed_swap_out,
    .destroy = file_backed_destroy,
    .type = VM_FILE,
};

/* The initializer of file vm */
void vm_file_init(void) {
    lock_init(&file_swap_lock);
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

    struct load_info *info = (struct load_info *)page->uninit.aux;
    uint8_t *kpage = page->frame->kva;
    if (kpage == NULL)
        return false;
    file_seek(info->file, info->offset);

    /* Load this page. */
    lock_acquire(&file_swap_lock);
    file_seek(info->file, info->offset);
    file_read(info->file, kpage, info->page_read_bytes);
    lock_release(&file_swap_lock);
    return true;
}

/* Swap out the page by writeback contents to the file. */
static bool file_backed_swap_out(struct page *page) {
    struct file_page *file_page UNUSED = &page->file;
    if (!page)
        return false;

    struct load_info *info = page->uninit.aux;

    lock_acquire(&file_swap_lock);

    if (pml4_is_dirty(thread_current()->pml4, page->va)) {
        if (file_write_at(info->file, page->frame->kva, info->page_read_bytes,
                          info->offset) != (int)info->page_read_bytes) {
            lock_release(&file_swap_lock);
            return false;
        }
        pml4_set_dirty(thread_current()->pml4, page->va, 0);
    }
    pml4_clear_page(thread_current()->pml4, page->va);
    page->frame->page = NULL;
    page->frame = NULL;
    lock_release(&file_swap_lock);
    return true;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void file_backed_destroy(struct page *page) {
    struct file_page *file_page UNUSED = &page->file;
    struct load_info *info = page->uninit.aux;

    lock_acquire(&file_swap_lock);
    if (pml4_is_dirty(thread_current()->pml4, page->va)) {
        if (file_write_at(info->file, page->frame->kva, info->page_read_bytes, info->offset) != (int)info->page_read_bytes) {
            lock_release(&file_swap_lock);
            return false;
        }
        pml4_set_dirty(thread_current()->pml4, page->va, 0);
    }
    lock_release(&file_swap_lock);

    if (page->frame && page->frame->page == page) {
        lock_acquire(&file_swap_lock);
        free_frame(page->frame);
        lock_release(&file_swap_lock);
    }
    pml4_clear_page(thread_current()->pml4, page->va);
}

/* Do the mmap */
void *
do_mmap(void *addr, size_t length, int writable,
        struct file *file, off_t offset) {
    void *ret = addr;
    struct file *mapping_file = file_reopen(file);
    size_t read_bytes = length;
    size_t zero_bytes = PGSIZE - (length % PGSIZE);
    while (read_bytes > 0 || zero_bytes > 0) {
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        /* TODO: Set up aux to pass information to the lazy_load_segment. */
        struct load_info *info = malloc(sizeof *info);
        if (!info)
            return NULL;
        info->file = mapping_file;
        info->page_read_bytes = page_read_bytes;
        info->page_zero_bytes = page_zero_bytes;
        info->offset = offset;
        if (!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_file, info)) {
            return NULL;
        }
        /* Advance. */
        read_bytes -= page_read_bytes;
        if (!read_bytes) {
            struct page *last_file_page = spt_find_page(&thread_current()->spt, addr);
            last_file_page->is_last_file_page = true;
        }
        zero_bytes -= page_zero_bytes;
        addr += PGSIZE;
        offset += page_read_bytes;
    }
    return ret;
}

/* Do the munmap */
void do_munmap(void *addr) {
    struct page *page;
    struct load_info *info;
    size_t file_size;
    off_t offset;
    page = spt_find_page(&thread_current()->spt, addr);

    if (!page)
        return;

    while (page) {
        info = page->uninit.aux;

        if (pml4_is_dirty(thread_current()->pml4, page->va)) {
            lock_acquire(&file_swap_lock);
            file_seek(info->file, info->offset);
            file_write(info->file, page->frame->kva, info->page_read_bytes);
            lock_release(&file_swap_lock);
            pml4_set_dirty(thread_current()->pml4, page->va, 0);
        }
        pml4_clear_page(thread_current()->pml4, page->va);
        /* Advance. */

        addr += PGSIZE;
        spt_remove_page(&thread_current()->spt, page);
        page = spt_find_page(&thread_current()->spt, addr);
    }
    file_close(info->file);
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
    lock_acquire(&file_swap_lock);
    file_seek(info->file, info->offset);

    /* Load this page. */
    file_read(info->file, kpage, info->page_read_bytes);
    lock_release(&file_swap_lock);

    return true;
}