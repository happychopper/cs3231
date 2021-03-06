/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#include <types.h>
#include <proc.h>
#include <synch.h>

/*
 * Put your function declarations and data types here ...
 */

// the struct of the openfile table
struct open_file_info{
    // current offset of this open file
    off_t f_offset;
    // how much time this slot has referenced
    unsigned int ref_count;
    // the pointer of this openfile vnode  
    struct vnode *vn;
    // how doeqqqs this file opened
    int o_flags;
    // A lock may be needed to store here 
    // to give an atomic operation of file operation.
    struct lock * f_lock;
};

// According to the piazza 
// https://piazza.com/class/jdwg14qxhhb4kp?cid=230
// the file table has construct in this number
// https://piazza.com/class/jdwg14qxhhb4kp?cid=226
// A big enough table is enough for this table
// since all the iov is maxed by this number, we can assume
// no that much file is in the table. (Need more justificatiion)
struct open_file_info *of_table[2*__IOV_MAX];


#define STR_BUF_SIZE __ARG_MAX

// a global buffer to record the filename or read info 
char str_buf[STR_BUF_SIZE];


int ker_open(char * filename, int flags, mode_t mode, int *retval, struct proc *to_proc );
int ker__close(int fd, struct proc *to_proc );

int sys__open(userptr_t filename, int flags, mode_t mode,int *retval);
int sys__read(int fd, void * buf, size_t buflen, size_t * retval );
int sys__write(int fd, void * buf, size_t nbytes , size_t *retval);
int sys__lseek(int fd, off_t pos, int whence,off_t *retval64);
int sys__close(int fd);
int sys__dup2(int oldfd, int newfd, int *retval);
// a local use function.
void set_lock_name (int ofi_id);

#endif /* _FILE_H_ */
