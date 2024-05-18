/* vm.c: Generic interface for virtual memory objects. */

#include "vm/vm.h"
#include "threads/malloc.h"
#include "vm/inspect.h"
#include "threads/mmu.h"
#include <string.h>

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void) {
    vm_anon_init();
    vm_file_init();
#ifdef EFILESYS /* For project 4 */
    pagecache_init();
#endif
    register_inspect_intr();
    /* DO NOT MODIFY UPPER LINES. */
    /* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type(struct page *page) {
    int ty = VM_TYPE(page->operations->type);
    switch (ty) {
    case VM_UNINIT:
        return VM_TYPE(page->uninit.type);
    default:
        return ty;
    }
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);
static void hash_destroy_support(struct hash_elem *e, void *aux);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
                                    vm_initializer *init, void *aux) {

    ASSERT(VM_TYPE(type) != VM_UNINIT)

    struct supplemental_page_table *spt = &thread_current()->spt;

    /* Check wheter the upage is already occupied or not. */
    if (spt_find_page(spt, upage) == NULL) {
        /* TODO: Create the page, fetch the initialier according to the VM type,
         * TODO: and then create "uninit" page struct by calling uninit_new. You
         * TODO: should modify the field after calling the uninit_new. */
        struct page *page = calloc(1, sizeof *page);

        switch (VM_TYPE(type)) {
        case VM_ANON:
            uninit_new(page, upage, init, type, aux, anon_initializer);
            break;
        case VM_FILE:
            uninit_new(page, upage, init, type, aux, file_backed_initializer);
            break;
        default:
            break;
        }
        page->writable = writable;

        /* TODO: Insert the page into the spt. */
        return spt_insert_page(spt, page);
    }
err:
    return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
    struct page *page = NULL;
    struct page _page;
    _page.va = pg_round_down(va);
    struct hash_elem *find_elem = hash_find(&spt->pages, &_page.hash_elem);
    /* TODO: Fill this function. */
    if (!find_elem)
        return NULL;

    page = hash_entry(find_elem, struct page, hash_elem);
    return page;
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
                     struct page *page UNUSED) {
    /* TODO: Fill this function. */
    return !hash_insert(&spt->pages, &page->hash_elem);
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page) {
    vm_dealloc_page(page);
    return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void) {
    struct frame *victim = NULL;
    /* TODO: The policy for eviction is up to you. */

    return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void) {
    struct frame *victim UNUSED = vm_get_victim();
    /* TODO: swap out the victim and return the evicted frame. */

    return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame(void) {
    struct frame *frame = calloc(1, sizeof *frame);
    frame->kva = palloc_get_page(PAL_ZERO | PAL_USER);
    /* TODO: Fill this function. */
    if (frame->kva == NULL)
        PANIC("todo");
    ASSERT(frame != NULL);
    ASSERT(frame->page == NULL);
    return frame;
}

/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED) {
    vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), true);

    if (!vm_claim_page(addr)) {
        PANIC("todo");
    }
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED) {
}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
                         bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
    struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
    struct page *page = NULL;

    if (!not_present)
        return false; // copy-on-wrtie 구현하면 여기서 함수 호출;
    /* TODO: Validate the fault */
    /* TODO: Your code goes here */
    page = spt_find_page(spt, addr);

    if (!page) {

        if (pg_round_down(addr) <= USER_STACK + PGSIZE - (1 << 20))
            return false;

        page = spt_find_page(spt, pg_round_up(addr));

        if (page && ((page->uninit.type) & VM_MARKER_0) && addr == thread_current()->user_rsp) {
            vm_stack_growth(addr);
            return true;
        }
        return false;
    }

    return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page) {
    destroy(page);
    free(page);
}

/* Claim the page that allocate on VA. */
bool vm_claim_page(void *va UNUSED) {
    struct thread *curr = thread_current();
    struct page *page = spt_find_page(&curr->spt, va);
    /* TODO: Fill this function */
    if (!page)
        return false;
    return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page) {
    struct frame *frame = vm_get_frame();
    struct thread *curr = thread_current();
    bool succ;
    /* Set links */
    frame->page = page;
    page->frame = frame;

    /* TODO: Insert page table entry to map page's VA to frame's PA. */
    succ = pml4_set_page(curr->pml4, page->va, frame->kva, page->writable);
    if (!succ)
        return succ;

    return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED) {
    hash_init(&spt->pages, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
                                  struct supplemental_page_table *src UNUSED) {
    struct hash_iterator i;
    hash_first(&i, &src->pages);
    while (hash_next(&i)) {
        struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
        struct page *dst_page;
        enum vm_type src_type = VM_TYPE(src_page->operations->type);

        if (src_type == VM_UNINIT) {
            if (!vm_alloc_page_with_initializer(src_page->uninit.type, src_page->va, src_page->writable, src_page->uninit.init, src_page->uninit.aux)) {
                return false;
            }
            continue;
        }

        vm_alloc_page(src_type, src_page->va, src_page->writable);

        vm_claim_page(src_page->va);
        dst_page = spt_find_page(dst, src_page->va);
        memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
    }
    return true;
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED) {
    /* TODO: Destroy all the supplemental_page_table hold by thread and
     * TODO: writeback all the modified contents to the storage. */
    hash_clear(&spt->pages, hash_destroy_support);
}

static void hash_destroy_support(struct hash_elem *e, void *aux) {
    struct page *p = hash_entry(e, struct page, hash_elem);
    vm_dealloc_page(p);
}

unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED) {
    const struct page *p = hash_entry(p_, struct page, hash_elem);
    return hash_bytes(&p->va, sizeof p->va);
}

bool page_less(const struct hash_elem *a_,
               const struct hash_elem *b_, void *aux UNUSED) {
    const struct page *a = hash_entry(a_, struct page, hash_elem);
    const struct page *b = hash_entry(b_, struct page, hash_elem);

    return a->va < b->va;
}