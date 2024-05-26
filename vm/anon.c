/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "devices/disk.h"
#include "vm/vm.h"
#include "lib/kernel/bitmap.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

extern struct lock frame_lock;
struct lock swap_lock;

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
struct bitmap *sdt;
static bool anon_swap_in(struct page *page, void *kva);
static bool anon_swap_out(struct page *page);
static void anon_destroy(struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
    .swap_in = anon_swap_in,
    .swap_out = anon_swap_out,
    .destroy = anon_destroy,
    .type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void vm_anon_init(void) {
    /* TODO: Set up the swap_disk. */
    swap_disk = disk_get(1, 1);
    size_t swap_size = disk_size(swap_disk) / (PGSIZE / DISK_SECTOR_SIZE);
    sdt = bitmap_create(swap_size); // 전체 slot 수
    bitmap_set_all(sdt, true);
    lock_init(&swap_lock);
}

/* Initialize the file mapping */
bool anon_initializer(struct page *page, enum vm_type type, void *kva) {
    /* Set up the handler */
    page->operations = &anon_ops;

    struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in(struct page *page, void *kva) {
    struct anon_page *anon_page = &page->anon;
    lock_acquire(&swap_lock);
    for (int i = 0; i < 8; i++) {
        disk_read(swap_disk, (page->slot_idx) * 8 + i, kva + (i * DISK_SECTOR_SIZE));
    }
    bitmap_flip(sdt, page->slot_idx);
    lock_release(&swap_lock);
    return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out(struct page *page) {
    struct anon_page *anon_page = &page->anon;
    lock_acquire(&swap_lock);
    size_t slot_idx = bitmap_scan_and_flip(sdt, 0, 1, true);
    if (slot_idx == BITMAP_ERROR) {
        lock_release(&swap_lock);
        return false;
    }
    lock_release(&swap_lock);

    page->slot_idx = slot_idx;
    lock_acquire(&swap_lock);

    for (int i = 0; i < 8; i++) {
        disk_write(swap_disk, slot_idx * 8 + i, (page->frame->kva) + (i * DISK_SECTOR_SIZE));
    }

    lock_release(&swap_lock);
    pml4_clear_page(thread_current()->pml4, page->va);

    return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy(struct page *page) {
    struct anon_page *anon_page = &page->anon;
    lock_acquire(&swap_lock);
    bitmap_set(sdt, page->slot_idx, true);
    lock_release(&swap_lock);
    lock_acquire(&frame_lock);
    list_remove(&page->frame->elem);
    lock_release(&frame_lock);
}
