#include "multiplexer.h"
// FIXME: Multiplexer cleanup logic.
void multiplexer_add(Multiplexer* mp, Inode* inode) {
    if(!mp || !inode) return;
    rwlock_begin_write(&mp->lock);
    
    // Check if already in list (prevent double-add)
    for(struct list *head = mp->list.next; head != &mp->list; head = head->next) {
        if((Inode*)head == inode) {
            rwlock_end_write(&mp->lock);
            return;  // Already added
        }
    }   
    list_append(&mp->list, &inode->list);
    rwlock_end_write(&mp->lock);
}
Multiplexer keyboard_mp = { 0 };
Multiplexer mouse_mp = { 0 };
static intptr_t multiplexer_read(Inode* file, void* buf, size_t size, off_t offset) {
    (void)offset;
    Multiplexer* mp = file->priv;
    size_t n = 0;
    intptr_t e;
    rwlock_begin_read(&mp->lock);
    for(struct list *head = mp->list.next; head != &mp->list && n < size;) {
        Inode* inode = (Inode*)head;
        head = head->next;  // Save next BEFORE reading (inode might be removed)   
        if((e = inode_read(inode, buf + n, size - n, 0)) < 0) {
            if(e == -WOULD_BLOCK) continue;
            rwlock_end_read(&mp->lock);
            if(n == 0) return e;
            return n;
        }
        n += (size_t)e;
    }   
    rwlock_end_read(&mp->lock);
    return n;
}
static bool multiplexer_is_readable(Inode* file) {
    Multiplexer* mp = file->priv;
    rwlock_begin_read(&mp->lock);
    for(struct list *head = mp->list.next; head != &mp->list; head = head->next) {
        Inode* inode = (Inode*)head;
        if(inode_is_readable(inode)) {
            rwlock_end_read(&mp->lock);
            return true;
        }
    }   
    rwlock_end_read(&mp->lock);
    return false;
}
static InodeOps inodeOps = {
    .read = multiplexer_read,
    .is_readable = multiplexer_is_readable,
};
intptr_t init_multiplexers(void) {
    list_init(&keyboard_mp.list);
    rwlock_init(&keyboard_mp.lock);
    list_init(&mouse_mp.list);
    rwlock_init(&mouse_mp.lock);
    Inode* keyboard = new_inode();
    if(!keyboard) return -NOT_ENOUGH_MEM;  // Check for failure
    keyboard->priv = &keyboard_mp;
    keyboard->ops = &inodeOps;
    Inode* mouse = new_inode();
    if(!mouse) {
        idrop(keyboard);  // Cleanup
        return -NOT_ENOUGH_MEM;
    }
    mouse->priv = &mouse_mp;
    mouse->ops = &inodeOps;
    intptr_t e;
    if((e = vfs_register_device("keyboard", keyboard)) < 0) {
        idrop(mouse);
        return e;
    }
    if((e = vfs_register_device("mouse", mouse)) < 0) {
        return e;
    }
    return 0;
}
